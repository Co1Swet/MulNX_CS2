/*
 * Copyright (c) 2026 Michael Niedermayer <michael-ffmpeg@niedermayer.cc>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Convert packed RGB/BGR to PAL8 with a per-frame palette whose colors sit
 * on a face-centered cubic (FCC) lattice.
 *
 * The FCC lattice is realized as the D3 checkerboard lattice: the points
 * (i,j,k) with an even coordinate sum, scaled so that the density option is
 * the number of lattice steps spanning one color axis (0..255). Only the
 * lattice points actually used by a frame enter its palette; if a frame
 * uses more than 256 of them, the used colors are reduced by iteratively
 * dropping the color whose removal has the least impact and mapping its
 * pixels to the nearest remaining color.
 */

#include <math.h>
#include "libavutil/lfg.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"
#include "filters.h"
#include "formats.h"
#include "video.h"

#define MAX_DENSITY 255
#define MAX_DENSITY_ALPHA 64

enum dithering_mode {
    DITHERING_NONE,
    DITHERING_BAYER,
    DITHERING_BLUE_NOISE,
    DITHERING_FLOYD_STEINBERG,
    NB_DITHERING
};

enum refine_mode {
    REFINE_NONE,
    REFINE_FULL,
    REFINE_RESIDUAL,
    REFINE_BATCHED,
    NB_REFINE
};

/* void-and-cluster blue noise mask parameters */
#define VC_SHIFT  6
#define VC_SIZE   (1 << VC_SHIFT)
#define VC_AREA   (VC_SIZE * VC_SIZE)
#define VC_MASK   (VC_SIZE - 1)
#define VC_SIGMA  1.5f
#define VC_RADIUS 7
#define VC_KSIZE  (2 * VC_RADIUS + 1)

typedef struct PalEntry {
    int pos;            ///< lattice cell position, ((i * dim + j) * dim + k) [* dim + l]
    uint8_t ci[4];      ///< lattice indices of the color
    uint8_t alive;      ///< still part of the palette while reducing
    int nn;             ///< nearest live color while reducing, then palette slot
    int d2;             ///< squared RGB(A) distance to nn
    int64_t count;      ///< pixels quantizing to this color, incl. absorbed ones
} PalEntry;

typedef struct LatticePalContext {
    const AVClass *class;
    int density;                        ///< lattice steps per color axis
    int dither;
    int max_colors;                     ///< palette entries to use at most
    int alpha;                          ///< quantize the alpha channel too (D4 lattice)
    int refine;                         ///< rediffuse the error of dropped colors

    int ro, go, bo, ao;                 ///< byte offsets of R, G, B, A in an input pixel, ao < 0: opaque
    int nc;                             ///< quantized components, 3 or 4
    int pixstep;                        ///< bytes per input pixel
    float scale;                        ///< density / 255
    uint8_t idx2val[MAX_DENSITY + 1];   ///< lattice index -> 8-bit component value
    int ordered_dither[4][8 * 8];       ///< per-channel bayer offsets spanning one lattice period
    int *blue_dither[4];                ///< per-channel blue noise offsets spanning one lattice period
    int32_t *err[2];                    ///< Floyd-Steinberg error rows, nc ints per pixel

    int dim;                            ///< density + 1, lattice cells per axis
    int min_gap;                        ///< smallest idx2val increment
    int32_t *cell;                      ///< dim^nc table: lattice cell -> list index + 1, 0 if unused
    uint32_t *pixpos;                   ///< per-pixel cell position of the quantized color
    PalEntry *list;                     ///< colors used by the current frame
    int nb_used;
    int nb_alloc;
    int *alive_arr;                     ///< compact list of live color indices while reducing
    int *alive_pos;                     ///< position of each live color in alive_arr
    int nb_alive;                       ///< entries in alive_arr, 0 outside reduction/remap
    int alive_alloc;
    uint32_t prev_pal[AVPALETTE_COUNT]; ///< previous frame's palette, for stable slot assignment
    int *touched;                       ///< cells filled on demand by the refinement pass
    int nb_touched;
    int touched_alloc;
} LatticePalContext;

#define OFFSET(x) offsetof(LatticePalContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM
static const AVOption latticepal_options[] = {
    { "density", "set the number of lattice steps spanning one color axis", OFFSET(density), AV_OPT_TYPE_INT, {.i64=20}, 1, MAX_DENSITY, FLAGS },
    { "max_colors", "set the maximum number of palette entries to use", OFFSET(max_colors), AV_OPT_TYPE_INT, {.i64=256}, 2, 256, FLAGS },
    { "alpha", "quantize the alpha channel too, on a 4 dimensional lattice", OFFSET(alpha), AV_OPT_TYPE_BOOL, {.i64=0}, 0, 1, FLAGS },
    { "refine", "rediffuse the error of dropped colors against the final palette", OFFSET(refine), AV_OPT_TYPE_INT, {.i64=REFINE_NONE}, 0, NB_REFINE-1, FLAGS, .unit = "refine_mode" },
        { "none",     "no refinement",                                        0, AV_OPT_TYPE_CONST, {.i64=REFINE_NONE},     INT_MIN, INT_MAX, FLAGS, .unit = "refine_mode" },
        { "full",     "rediffuse the whole frame against the final palette",  0, AV_OPT_TYPE_CONST, {.i64=REFINE_FULL},     INT_MIN, INT_MAX, FLAGS, .unit = "refine_mode" },
        { "residual", "diffuse only the residual of the dropped colors",      0, AV_OPT_TYPE_CONST, {.i64=REFINE_RESIDUAL}, INT_MIN, INT_MAX, FLAGS, .unit = "refine_mode" },
        { "batched",  "interleave removal and rediffusion in geometric batches", 0, AV_OPT_TYPE_CONST, {.i64=REFINE_BATCHED}, INT_MIN, INT_MAX, FLAGS, .unit = "refine_mode" },
    { "dither", "select dithering mode", OFFSET(dither), AV_OPT_TYPE_INT, {.i64=DITHERING_FLOYD_STEINBERG}, 0, NB_DITHERING-1, FLAGS, .unit = "dithering_mode" },
        { "none",            "no dithering",                    0, AV_OPT_TYPE_CONST, {.i64=DITHERING_NONE},            INT_MIN, INT_MAX, FLAGS, .unit = "dithering_mode" },
        { "bayer",           "ordered 8x8 bayer dithering",     0, AV_OPT_TYPE_CONST, {.i64=DITHERING_BAYER},           INT_MIN, INT_MAX, FLAGS, .unit = "dithering_mode" },
        { "blue_noise",      "void-and-cluster blue noise dithering", 0, AV_OPT_TYPE_CONST, {.i64=DITHERING_BLUE_NOISE}, INT_MIN, INT_MAX, FLAGS, .unit = "dithering_mode" },
        { "floyd_steinberg", "Floyd-Steinberg error diffusion", 0, AV_OPT_TYPE_CONST, {.i64=DITHERING_FLOYD_STEINBERG}, INT_MIN, INT_MAX, FLAGS, .unit = "dithering_mode" },
    { NULL }
};

AVFILTER_DEFINE_CLASS(latticepal);

static int query_formats(const AVFilterContext *ctx,
                         AVFilterFormatsConfig **cfg_in,
                         AVFilterFormatsConfig **cfg_out)
{
    static const enum AVPixelFormat in_fmts[] = {
        AV_PIX_FMT_RGB24, AV_PIX_FMT_BGR24,
        AV_PIX_FMT_RGBA,  AV_PIX_FMT_BGRA,
        AV_PIX_FMT_ARGB,  AV_PIX_FMT_ABGR,
        AV_PIX_FMT_RGB0,  AV_PIX_FMT_BGR0,
        AV_PIX_FMT_0RGB,  AV_PIX_FMT_0BGR,
        AV_PIX_FMT_NONE
    };
    static const enum AVPixelFormat out_fmts[] = { AV_PIX_FMT_PAL8, AV_PIX_FMT_NONE };
    int ret;

    if ((ret = ff_formats_ref(ff_make_pixel_format_list(in_fmts),  &cfg_in[0]->formats)) < 0)
        return ret;
    if ((ret = ff_formats_ref(ff_make_pixel_format_list(out_fmts), &cfg_out[0]->formats)) < 0)
        return ret;
    return 0;
}

/* Classic swscale ordered dither matrix (libswscale/output.c), 73 levels. */
static const uint8_t dither_8x8_73[8][8] = {
    {  0, 55, 14, 68,  3, 58, 17, 72, },
    { 37, 18, 50, 32, 40, 22, 54, 35, },
    {  9, 64,  5, 59, 13, 67,  8, 63, },
    { 46, 27, 41, 23, 49, 31, 44, 26, },
    {  2, 57, 16, 71,  1, 56, 15, 70, },
    { 39, 21, 52, 34, 38, 19, 51, 33, },
    { 11, 66,  7, 62, 10, 65,  6, 60, },
    { 48, 30, 43, 25, 47, 29, 42, 24, },
};

/**
 * Add (sign > 0) or remove (sign < 0) the Gaussian energy contribution of a
 * minority pixel at position pos on the VC_SIZE x VC_SIZE torus.
 */
static void vc_splat(float *energy, const float *kern, int pos, float sign)
{
    const int px = pos & VC_MASK, py = pos >> VC_SHIFT;

    for (int dy = -VC_RADIUS; dy <= VC_RADIUS; dy++) {
        const int y = (py + dy) & VC_MASK;
        const float *krow = &kern[(dy + VC_RADIUS) * VC_KSIZE + VC_RADIUS];
        float *erow = &energy[y << VC_SHIFT];

        for (int dx = -VC_RADIUS; dx <= VC_RADIUS; dx++)
            erow[(px + dx) & VC_MASK] += sign * krow[dx];
    }
}

/**
 * Position of the extreme energy value among the cells whose bit equals
 * want_set: the tightest cluster (find_max, among minority pixels) or the
 * largest void (!find_max, among majority pixels).
 */
static int vc_extreme(const float *energy, const uint8_t *bits, int want_set, int find_max)
{
    int best = -1;
    float beste = 0.f;

    for (int i = 0; i < VC_AREA; i++) {
        if (bits[i] != want_set)
            continue;
        if (best < 0 || (find_max ? energy[i] > beste : energy[i] < beste)) {
            best  = i;
            beste = energy[i];
        }
    }
    return best;
}

/**
 * Generate a void-and-cluster blue noise rank matrix (Ulichney 1993):
 * every cell gets a unique rank in [0, VC_AREA).
 *
 * On a torus the filtered field of the zero pattern is the constant kernel
 * sum minus the field of the one pattern, so the tightest cluster of zeros
 * coincides with the largest void of ones and the ranking above 50% fill
 * needs no separate phase.
 */
static void vc_generate(uint16_t *rank, uint8_t *bits, float *energy,
                        float *e1, const float *kern, AVLFG *lfg)
{
    const int n1 = VC_AREA / 10;
    uint8_t b1[VC_AREA];
    int placed, pos;

    memset(bits,   0, VC_AREA * sizeof(*bits));
    memset(energy, 0, VC_AREA * sizeof(*energy));

    /* initial random minority pattern */
    for (placed = 0; placed < n1;) {
        pos = av_lfg_get(lfg) & (VC_AREA - 1);
        if (!bits[pos]) {
            bits[pos] = 1;
            vc_splat(energy, kern, pos, 1.f);
            placed++;
        }
    }

    /* relax: move the tightest cluster into the largest void until stable */
    for (int i = 0; i < VC_AREA * 8; i++) {
        pos = vc_extreme(energy, bits, 1, 1);
        bits[pos] = 0;
        vc_splat(energy, kern, pos, -1.f);
        const int vp = vc_extreme(energy, bits, 0, 0);
        bits[vp] = 1;
        vc_splat(energy, kern, vp, 1.f);
        if (vp == pos)
            break;
    }

    /* phase 1: rank the initial pattern by removing tightest clusters */
    memcpy(b1, bits,   sizeof(b1));
    memcpy(e1, energy, VC_AREA * sizeof(*e1));
    for (int r = n1 - 1; r >= 0; r--) {
        pos = vc_extreme(e1, b1, 1, 1);
        b1[pos] = 0;
        vc_splat(e1, kern, pos, -1.f);
        rank[pos] = r;
    }

    /* phase 2+3: fill the largest void until all cells are ranked */
    for (int r = n1; r < VC_AREA; r++) {
        pos = vc_extreme(energy, bits, 0, 0);
        bits[pos] = 1;
        vc_splat(energy, kern, pos, 1.f);
        rank[pos] = r;
    }
}

/**
 * Quantize a color to the nearest point of the scaled D3 (FCC) or D4
 * lattice.
 *
 * Conway & Sloane: round every coordinate to the nearest integer; if the
 * coordinate sum is odd, re-round the coordinate with the largest rounding
 * error in the other direction. This works for any checkerboard lattice Dn.
 *
 * @param in the nc input components
 * @param f  receives the nc lattice indices
 */
static av_always_inline void quant_dn(const LatticePalContext *s, const int *in, int f[4])
{
    const int N = s->density;
    const int nc = s->nc;
    int sum = 0;
    float d[4];

    for (int i = 0; i < nc; i++) {
        const float u = av_clipf(in[i] * s->scale, 0.f, N);
        f[i] = lrintf(u);
        d[i] = u - f[i];
        sum += f[i];
    }

    if (sum & 1) {
        int k = 0, dir;
        for (int i = 1; i < nc; i++)
            if (fabsf(d[i]) > fabsf(d[k]))
                k = i;
        dir = d[k] >= 0.f ? 1 : -1;
        if (f[k] + dir < 0 || f[k] + dir > N)
            dir = -dir;
        f[k] += dir;
    }
}

/**
 * Account one pixel using the lattice color with indices f, registering the
 * color in the cell table and the used color list on first use.
 *
 * @return the lattice cell position, or AVERROR(ENOMEM)
 */
static av_always_inline int color_inc(LatticePalContext *s, const int f[4])
{
    int pos = f[0];
    int idx;

    for (int i = 1; i < s->nc; i++)
        pos = pos * s->dim + f[i];
    idx = s->cell[pos];

    if (!idx) {
        PalEntry *e;

        if (s->nb_used == s->nb_alloc) {
            const int na = FFMAX(s->nb_alloc * 2, 512);
            PalEntry *nl = av_realloc_array(s->list, na, sizeof(*nl));
            if (!nl)
                return AVERROR(ENOMEM);
            s->list     = nl;
            s->nb_alloc = na;
        }
        e = &s->list[s->nb_used];
        e->pos   = pos;
        e->ci[0] = f[0];
        e->ci[1] = f[1];
        e->ci[2] = f[2];
        e->ci[3] = s->nc == 4 ? f[3] : 0;
        e->alive = 1;
        e->count = 0;
        s->cell[pos] = idx = ++s->nb_used;
    }
    s->list[idx - 1].count++;
    return pos;
}

/**
 * Find the nearest live color of the used color list, excluding self
 * (pass -1 to match any live color).
 *
 * While many colors are alive, search outward in Chebyshev shells of the
 * lattice index space; the component value difference of a cell r shells
 * away is at least r * min_gap, which bounds the search once a candidate
 * is known. When only few colors are left the lattice around them is
 * sparse and shells get expensive, so scan the compact live list instead.
 * (For 3 components ci[3] is 0 everywhere, contributing nothing.)
 *
 * @return list index of the nearest live color, its distance in *out_d2
 */
static int nearest_alive(LatticePalContext *s, const uint8_t ci[4], int self, int *out_d2)
{
    PalEntry *list = s->list;
    const uint8_t *val = s->idx2val;
    const int dim = s->dim;
    const int i0 = ci[0], i1 = ci[1], i2 = ci[2], i3 = ci[3];
    const int v0 = val[i0], v1 = val[i1], v2 = val[i2], v3 = val[i3];
    int best = -1, bestd = INT_MAX;

    if (s->nb_alive > 0 && s->nb_alive <= 1024) {
        for (int k = 0; k < s->nb_alive; k++) {
            const int j = s->alive_arr[k];
            const PalEntry *o = &list[j];
            int d2, dv;

            if (j == self)
                continue;
            dv = val[o->ci[0]] - v0; d2  = dv * dv;
            dv = val[o->ci[1]] - v1; d2 += dv * dv;
            dv = val[o->ci[2]] - v2; d2 += dv * dv;
            dv = val[o->ci[3]] - v3; d2 += dv * dv;
            if (d2 < bestd) {
                bestd = d2;
                best  = j;
            }
        }
        *out_d2 = best >= 0 ? bestd : 0;
        return best;
    }

#define CHECK_CELL3(a, b, c) do {                                       \
    const int e = s->cell[((a) * dim + (b)) * dim + (c)];               \
    if (e > 0 && e - 1 != self && list[e - 1].alive) {                  \
        const int dr = val[a] - v0;                                     \
        const int dg = val[b] - v1;                                     \
        const int db = val[c] - v2;                                     \
        const int d2 = dr * dr + dg * dg + db * db;                     \
        if (d2 < bestd) {                                               \
            bestd = d2;                                                 \
            best  = e - 1;                                              \
        }                                                               \
    }                                                                   \
} while (0)

#define CHECK_CELL4(a, b, c, l) do {                                    \
    const int e = s->cell[(((a) * dim + (b)) * dim + (c)) * dim + (l)]; \
    if (e > 0 && e - 1 != self && list[e - 1].alive) {                  \
        const int dr = val[a] - v0;                                     \
        const int dg = val[b] - v1;                                     \
        const int db = val[c] - v2;                                     \
        const int da_ = val[l] - v3;                                    \
        const int d2 = dr * dr + dg * dg + db * db + da_ * da_;         \
        if (d2 < bestd) {                                               \
            bestd = d2;                                                 \
            best  = e - 1;                                              \
        }                                                               \
    }                                                                   \
} while (0)

    for (int r = 1; r < dim; r++) {
        if (best >= 0 && (int64_t)r * s->min_gap * r * s->min_gap > bestd)
            break;
        for (int a = FFMAX(i0 - r, 0); a <= FFMIN(i0 + r, dim - 1); a++) {
            const int da = FFABS(a - i0);
            for (int b = FFMAX(i1 - r, 0); b <= FFMIN(i1 + r, dim - 1); b++) {
                const int dab = FFMAX(da, FFABS(b - i1));

                if (s->nc == 3) {
                    if (dab == r) {
                        for (int c = FFMAX(i2 - r, 0); c <= FFMIN(i2 + r, dim - 1); c++)
                            if (!((a + b + c) & 1))
                                CHECK_CELL3(a, b, c);
                    } else {
                        if (i2 - r >= 0 && !((a + b + i2 - r) & 1))
                            CHECK_CELL3(a, b, i2 - r);
                        if (i2 + r < dim && !((a + b + i2 + r) & 1))
                            CHECK_CELL3(a, b, i2 + r);
                    }
                } else {
                    for (int c = FFMAX(i2 - r, 0); c <= FFMIN(i2 + r, dim - 1); c++) {
                        if (FFMAX(dab, FFABS(c - i2)) == r) {
                            for (int l = FFMAX(i3 - r, 0); l <= FFMIN(i3 + r, dim - 1); l++)
                                if (!((a + b + c + l) & 1))
                                    CHECK_CELL4(a, b, c, l);
                        } else {
                            if (i3 - r >= 0 && !((a + b + c + i3 - r) & 1))
                                CHECK_CELL4(a, b, c, i3 - r);
                            if (i3 + r < dim && !((a + b + c + i3 + r) & 1))
                                CHECK_CELL4(a, b, c, i3 + r);
                        }
                    }
                }
            }
        }
    }
#undef CHECK_CELL3
#undef CHECK_CELL4

    *out_d2 = best >= 0 ? bestd : 0;
    return best;
}

static void nn_search(LatticePalContext *s, int self)
{
    int d2;

    s->list[self].nn = nearest_alive(s, s->list[self].ci, self, &d2);
    s->list[self].d2 = d2;
}

typedef struct HeapEnt {
    int64_t imp;        ///< impact of the removal when the entry was pushed
    int idx;            ///< used color list index
} HeapEnt;

static void heap_push(HeapEnt *heap, int *nb, int64_t imp, int idx)
{
    int i = (*nb)++;

    while (i > 0 && heap[(i - 1) / 2].imp > imp) {
        heap[i] = heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap[i].imp = imp;
    heap[i].idx = idx;
}

static HeapEnt heap_pop(HeapEnt *heap, int *nb)
{
    const HeapEnt top = heap[0];
    const HeapEnt last = heap[--*nb];
    int i = 0, c;

    while ((c = 2 * i + 1) < *nb) {
        if (c + 1 < *nb && heap[c + 1].imp < heap[c].imp)
            c++;
        if (heap[c].imp >= last.imp)
            break;
        heap[i] = heap[c];
        i = c;
    }
    heap[i] = last;
    return top;
}

/**
 * Reduce the live colors down to target by repeatedly dropping the color
 * whose removal has the least impact: its pixel count times the squared
 * distance to the nearest remaining color, which then absorbs the
 * dropped pixels.
 *
 * An impact can only grow: counts grow by absorption and the nearest
 * neighbor distance grows when the neighbor is dropped. Stale heap entries
 * therefore underestimate their color's impact, and validating at pop time
 * and re-pushing the corrected entry yields the exact minimum.
 */
static int reduce_colors(LatticePalContext *s, int target)
{
    PalEntry *list = s->list;
    int alive;
    int nb_heap = 0, cap = s->nb_used + 64;
    HeapEnt *heap = av_malloc_array(cap, sizeof(*heap));

    if (!heap)
        return AVERROR(ENOMEM);

    if (s->alive_alloc < s->nb_used) {
        av_freep(&s->alive_arr);
        av_freep(&s->alive_pos);
        s->alive_arr = av_malloc_array(s->nb_used, sizeof(*s->alive_arr));
        s->alive_pos = av_malloc_array(s->nb_used, sizeof(*s->alive_pos));
        if (!s->alive_arr || !s->alive_pos) {
            av_free(heap);
            return AVERROR(ENOMEM);
        }
        s->alive_alloc = s->nb_used;
    }
    s->nb_alive = 0;
    for (int i = 0; i < s->nb_used; i++) {
        if (!list[i].alive)
            continue;
        s->alive_pos[i] = s->nb_alive;
        s->alive_arr[s->nb_alive++] = i;
    }
    alive = s->nb_alive;

    for (int k = 0; k < s->nb_alive; k++) {
        const int i = s->alive_arr[k];

        nn_search(s, i);
        heap_push(heap, &nb_heap, list[i].count * list[i].d2, i);
    }

    while (alive > target) {
        const HeapEnt top = heap_pop(heap, &nb_heap);
        const int i = top.idx;
        int64_t imp;

        if (!list[i].alive)
            continue;
        if (!list[list[i].nn].alive)
            nn_search(s, i);
        imp = list[i].count * list[i].d2;
        if (imp != top.imp) {
            /* stale entry, reinsert with the corrected impact */
            if (nb_heap == cap) {
                HeapEnt *nh = av_realloc_array(heap, cap * 2, sizeof(*heap));
                if (!nh) {
                    av_free(heap);
                    return AVERROR(ENOMEM);
                }
                heap = nh;
                cap *= 2;
            }
            heap_push(heap, &nb_heap, imp, i);
            continue;
        }
        list[i].alive = 0;
        list[list[i].nn].count += list[i].count;
        alive--;
        /* swap-remove from the compact live list */
        s->alive_arr[s->alive_pos[i]] = s->alive_arr[--s->nb_alive];
        s->alive_pos[s->alive_arr[s->nb_alive]] = s->alive_pos[i];
    }

    av_free(heap);
    return 0;
}

/** Palette color (AARRGGBB) of a used color list entry. */
static av_always_inline uint32_t entry_color(const LatticePalContext *s, const PalEntry *e)
{
    return (s->nc == 4 ? (uint32_t)s->idx2val[e->ci[3]] << 24 : 0xFF000000) |
           s->idx2val[e->ci[0]] << 16 |
           s->idx2val[e->ci[1]] <<  8 |
           s->idx2val[e->ci[2]];
}

/**
 * Full refinement pass: rediffuse the whole frame against the final
 * palette.
 *
 * The first pass diffused its error against the full lattice, so the shift
 * from dropped colors to their nearest survivor is uncompensated and shows
 * up as flat discolored patches exactly where the reduction hit. Re-run
 * Floyd-Steinberg error diffusion on the original pixels, quantizing to
 * the nearest final palette color, which redistributes that error by
 * construction (error diffusion is average correct against any codebook).
 *
 * The nearest palette color is resolved at lattice cell granularity: used
 * cells already hold their slot from the remap, cells first touched by the
 * diffusion get a scan over the at most 256 survivors and are cached in
 * the cell table and recorded for the per frame reset.
 */
static int refine_full(LatticePalContext *s, AVFrame *out, const AVFrame *in)
{
    const int w = in->width, h = in->height;
    const int ro = s->ro, go = s->go, bo = s->bo, ao = s->ao, pixstep = s->pixstep;
    const int nc = s->nc;
    const uint32_t *pal = (const uint32_t *)out->data[1];

    memset(s->err[0], 0, (w + 2) * nc * sizeof(*s->err[0]));
    memset(s->err[1], 0, (w + 2) * nc * sizeof(*s->err[1]));

    for (int y = 0; y < h; y++) {
        const uint8_t *src = in->data[0] + y * in->linesize[0];
        uint8_t *dst = out->data[0] + y * out->linesize[0];
        const int dir = (y & 1) ? -1 : 1;
        const int x0 = dir > 0 ? 0 : w - 1;
        int32_t *err_cur  = s->err[ y      & 1] + nc;
        int32_t *err_next = s->err[(y + 1) & 1] + nc;
        int f[4], px[4];

        px[3] = 255;
        memset(err_next - nc, 0, (w + 2) * nc * sizeof(*err_next));

        for (int k = 0; k < w; k++) {
            const int x = x0 + k * dir;
            const int32_t *e = &err_cur[x * nc];
            uint32_t c;
            int pos, slot1;

            px[0] = av_clip_uint8(src[x * pixstep + ro] + ((e[0] + 8) >> 4));
            px[1] = av_clip_uint8(src[x * pixstep + go] + ((e[1] + 8) >> 4));
            px[2] = av_clip_uint8(src[x * pixstep + bo] + ((e[2] + 8) >> 4));
            if (nc == 4)
                px[3] = av_clip_uint8((ao >= 0 ? src[x * pixstep + ao] : 255) + ((e[3] + 8) >> 4));

            quant_dn(s, px, f);
            pos = f[0];
            for (int i = 1; i < nc; i++)
                pos = pos * s->dim + f[i];

            slot1 = s->cell[pos];
            if (!slot1) {
                const uint8_t *val = s->idx2val;
                int bd = INT_MAX;

                for (int k = 0; k < s->nb_alive; k++) {
                    const PalEntry *o = &s->list[s->alive_arr[k]];
                    int d2, dv;

                    dv = val[o->ci[0]] - val[f[0]]; d2  = dv * dv;
                    dv = val[o->ci[1]] - val[f[1]]; d2 += dv * dv;
                    dv = val[o->ci[2]] - val[f[2]]; d2 += dv * dv;
                    if (nc == 4) {
                        dv = val[o->ci[3]] - val[f[3]]; d2 += dv * dv;
                    }
                    if (d2 < bd) {
                        bd    = d2;
                        slot1 = o->nn + 1;
                    }
                }
                if (s->nb_touched == s->touched_alloc) {
                    const int na = FFMAX(s->touched_alloc * 2, 1024);
                    int *nt = av_realloc_array(s->touched, na, sizeof(*nt));
                    if (!nt)
                        return AVERROR(ENOMEM);
                    s->touched = nt;
                    s->touched_alloc = na;
                }
                s->touched[s->nb_touched++] = pos;
                s->cell[pos] = slot1;
            }
            dst[x] = slot1 - 1;

            c = pal[slot1 - 1];
            for (int i = 0; i < nc; i++) {
                static const int sh[4] = { 16, 8, 0, 24 };
                const int qerr = px[i] - (int)(c >> sh[i] & 0xff);

                err_cur [(x + dir) * nc + i] += qerr * 7;
                err_next[(x - dir) * nc + i] += qerr * 3;
                err_next[ x        * nc + i] += qerr * 5;
                err_next[(x + dir) * nc + i] += qerr;
            }
        }
    }
    return 0;
}

/**
 * Residual refinement pass: rediffuse only the error of the dropped
 * colors, using the first pass quantized color as the diffusion target.
 * The error stream then carries only the removal residuals: a pixel whose
 * color survived and that receives no incoming residual picks its own
 * color with zero error and is unchanged, keeping the character of the
 * selected dither mode outside the reduced regions.
 *
 * Runs before the cell table is rewritten: cells hold list indices, and
 * cells resolved on demand are cached as -(slot + 1).
 */
static int refine_residual(LatticePalContext *s, AVFrame *out, const AVFrame *in)
{
    const int w = in->width, h = in->height;
    const int nc = s->nc;
    const uint32_t *pal = (const uint32_t *)out->data[1];
    const uint8_t *val = s->idx2val;
    PalEntry *list = s->list;

    memset(s->err[0], 0, (w + 2) * nc * sizeof(*s->err[0]));
    memset(s->err[1], 0, (w + 2) * nc * sizeof(*s->err[1]));

    for (int y = 0; y < h; y++) {
        const uint32_t *ppos = s->pixpos + (size_t)y * w;
        uint8_t *dst = out->data[0] + y * out->linesize[0];
        const int dir = (y & 1) ? -1 : 1;
        const int x0 = dir > 0 ? 0 : w - 1;
        int32_t *err_cur  = s->err[ y      & 1] + nc;
        int32_t *err_next = s->err[(y + 1) & 1] + nc;
        int f[4], px[4];

        px[3] = 255;
        memset(err_next - nc, 0, (w + 2) * nc * sizeof(*err_next));

        for (int k = 0; k < w; k++) {
            const int x = x0 + k * dir;
            const PalEntry *e1 = &list[s->cell[ppos[x]] - 1];
            const int32_t *e = &err_cur[x * nc];
            uint32_t c;
            int pos, v, slot;

            if (e1->alive && !(e[0] | e[1] | e[2] | (nc == 4 ? e[3] : 0))) {
                /* surviving color, no incoming residual: unchanged */
                dst[x] = e1->nn;
                continue;
            }

            /* target = first pass color + carried residual */
            for (int i = 0; i < nc; i++)
                px[i] = av_clip_uint8(val[e1->ci[i]] + ((e[i] + 8) >> 4));

            quant_dn(s, px, f);
            pos = f[0];
            for (int i = 1; i < nc; i++)
                pos = pos * s->dim + f[i];

            v = s->cell[pos];
            if (v > 0) {
                slot = list[v - 1].nn;
            } else if (v < 0) {
                slot = -v - 1;
            } else {
                int bd = INT_MAX;

                slot = 0;
                for (int j = 0; j < s->nb_alive; j++) {
                    const PalEntry *o = &list[s->alive_arr[j]];
                    int d2, dv;

                    dv = val[o->ci[0]] - val[f[0]]; d2  = dv * dv;
                    dv = val[o->ci[1]] - val[f[1]]; d2 += dv * dv;
                    dv = val[o->ci[2]] - val[f[2]]; d2 += dv * dv;
                    if (nc == 4) {
                        dv = val[o->ci[3]] - val[f[3]]; d2 += dv * dv;
                    }
                    if (d2 < bd) {
                        bd   = d2;
                        slot = o->nn;
                    }
                }
                if (s->nb_touched == s->touched_alloc) {
                    const int na = FFMAX(s->touched_alloc * 2, 1024);
                    int *nt = av_realloc_array(s->touched, na, sizeof(*nt));
                    if (!nt)
                        return AVERROR(ENOMEM);
                    s->touched = nt;
                    s->touched_alloc = na;
                }
                s->touched[s->nb_touched++] = pos;
                s->cell[pos] = -(slot + 1);
            }
            dst[x] = slot;

            c = pal[slot];
            for (int i = 0; i < nc; i++) {
                static const int sh[4] = { 16, 8, 0, 24 };
                const int qerr = px[i] - (int)(c >> sh[i] & 0xff);

                err_cur [(x + dir) * nc + i] += qerr * 7;
                err_next[(x - dir) * nc + i] += qerr * 3;
                err_next[ x        * nc + i] += qerr * 5;
                err_next[(x + dir) * nc + i] += qerr;
            }
        }
    }
    return 0;
}

/**
 * Rediffuse the whole frame against the current live colors and reassign
 * every pixel, recounting how often each live color is actually used.
 *
 * Cells resolving to a dead or unused state are cached as -(list index+1);
 * stale caches from earlier rounds heal themselves through the alive check.
 */
static int batch_assign(LatticePalContext *s, const AVFrame *in)
{
    const int w = in->width, h = in->height;
    const int ro = s->ro, go = s->go, bo = s->bo, ao = s->ao, pixstep = s->pixstep;
    const int nc = s->nc;
    const uint8_t *val = s->idx2val;
    PalEntry *list = s->list;

    for (int k = 0; k < s->nb_alive; k++)
        list[s->alive_arr[k]].count = 0;

    memset(s->err[0], 0, (w + 2) * nc * sizeof(*s->err[0]));
    memset(s->err[1], 0, (w + 2) * nc * sizeof(*s->err[1]));

    for (int y = 0; y < h; y++) {
        const uint8_t *src = in->data[0] + y * in->linesize[0];
        uint32_t *ppos = s->pixpos + (size_t)y * w;
        const int dir = (y & 1) ? -1 : 1;
        const int x0 = dir > 0 ? 0 : w - 1;
        int32_t *err_cur  = s->err[ y      & 1] + nc;
        int32_t *err_next = s->err[(y + 1) & 1] + nc;
        int f[4], px[4];

        px[3] = 255;
        memset(err_next - nc, 0, (w + 2) * nc * sizeof(*err_next));

        for (int k = 0; k < w; k++) {
            const int x = x0 + k * dir;
            const int32_t *e = &err_cur[x * nc];
            const PalEntry *o;
            int pos, v, j;

            px[0] = av_clip_uint8(src[x * pixstep + ro] + ((e[0] + 8) >> 4));
            px[1] = av_clip_uint8(src[x * pixstep + go] + ((e[1] + 8) >> 4));
            px[2] = av_clip_uint8(src[x * pixstep + bo] + ((e[2] + 8) >> 4));
            if (nc == 4)
                px[3] = av_clip_uint8((ao >= 0 ? src[x * pixstep + ao] : 255) + ((e[3] + 8) >> 4));

            quant_dn(s, px, f);
            pos = f[0];
            for (int i = 1; i < nc; i++)
                pos = pos * s->dim + f[i];

            v = s->cell[pos];
            if (v > 0 && list[v - 1].alive) {
                j = v - 1;
            } else if (v < 0 && list[-v - 1].alive) {
                j = -v - 1;
            } else {
                const uint8_t fci[4] = { f[0], f[1], f[2], nc == 4 ? f[3] : 0 };
                int d2;

                j = nearest_alive(s, fci, -1, &d2);
                if (!v) {
                    if (s->nb_touched == s->touched_alloc) {
                        const int na = FFMAX(s->touched_alloc * 2, 1024);
                        int *nt = av_realloc_array(s->touched, na, sizeof(*nt));
                        if (!nt)
                            return AVERROR(ENOMEM);
                        s->touched = nt;
                        s->touched_alloc = na;
                    }
                    s->touched[s->nb_touched++] = pos;
                }
                s->cell[pos] = -(j + 1);
            }
            o = &list[j];
            list[j].count++;
            ppos[x] = j;

            for (int i = 0; i < nc; i++) {
                const int qerr = px[i] - val[o->ci[i]];

                err_cur [(x + dir) * nc + i] += qerr * 7;
                err_next[(x - dir) * nc + i] += qerr * 3;
                err_next[ x        * nc + i] += qerr * 5;
                err_next[(x + dir) * nc + i] += qerr;
            }
        }
    }
    return 0;
}

/**
 * Batched reduction: instead of dropping all excess colors against the
 * first pass statistics, drop half of the excess, rediffuse the frame
 * against the survivors and recount from the actual assignments, then
 * repeat. Colors whose pixels the rediffusion absorbed elsewhere become
 * free to drop, while colors that dithering cannot reproduce (extremes of
 * the used gamut) keep their pixels and with them a high removal impact,
 * so they are protected in later rounds.
 */
static int reduce_batched(LatticePalContext *s, const AVFrame *in)
{
    int alive = s->nb_used;

    while (alive > s->max_colors) {
        const int excess = alive - s->max_colors;
        const int target = s->max_colors + excess / 2;
        int ret = reduce_colors(s, target);

        if (ret < 0)
            return ret;
        alive = target;

        ret = batch_assign(s, in);
        if (ret < 0)
            return ret;

        /* drop the colors the rediffusion no longer uses */
        for (int k = 0; k < s->nb_alive;) {
            const int i = s->alive_arr[k];

            if (!s->list[i].count) {
                s->list[i].alive = 0;
                s->alive_arr[k] = s->alive_arr[--s->nb_alive];
                s->alive_pos[s->alive_arr[k]] = k;
                alive--;
            } else
                k++;
        }
    }
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    LatticePalContext *s = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    const int w = in->width, h = in->height;
    const int ro = s->ro, go = s->go, bo = s->bo, ao = s->ao, pixstep = s->pixstep;
    const int nc = s->nc;
    uint32_t *pal;
    AVFrame *out;
    int ret, reduced = 0;

    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }
    ret = av_frame_copy_props(out, in);
    if (ret < 0)
        goto fail;

    s->nb_used  = 0;
    s->nb_alive = 0;
    if (s->dither == DITHERING_FLOYD_STEINBERG) {
        memset(s->err[0], 0, (w + 2) * nc * sizeof(*s->err[0]));
        memset(s->err[1], 0, (w + 2) * nc * sizeof(*s->err[1]));
    }

    /* first pass: quantize, collect the used colors and their pixel counts */
    for (int y = 0; y < h; y++) {
        const uint8_t *src = in->data[0] + y * in->linesize[0];
        uint32_t *ppos = s->pixpos + (size_t)y * w;
        int f[4], px[4], pos;

        px[3] = 255;

        if (s->dither == DITHERING_FLOYD_STEINBERG) {
            /* serpentine scan: odd lines run right to left with mirrored
             * diffusion weights, avoiding directional drift artifacts */
            const int dir = (y & 1) ? -1 : 1;
            const int x0 = dir > 0 ? 0 : w - 1;
            int32_t *err_cur  = s->err[ y      & 1] + nc;
            int32_t *err_next = s->err[(y + 1) & 1] + nc;

            memset(err_next - nc, 0, (w + 2) * nc * sizeof(*err_next));

            for (int k = 0; k < w; k++) {
                const int x = x0 + k * dir;
                const int32_t *e = &err_cur[x * nc];

                px[0] = av_clip_uint8(src[x * pixstep + ro] + ((e[0] + 8) >> 4));
                px[1] = av_clip_uint8(src[x * pixstep + go] + ((e[1] + 8) >> 4));
                px[2] = av_clip_uint8(src[x * pixstep + bo] + ((e[2] + 8) >> 4));
                if (nc == 4)
                    px[3] = av_clip_uint8((ao >= 0 ? src[x * pixstep + ao] : 255) + ((e[3] + 8) >> 4));

                quant_dn(s, px, f);
                pos = color_inc(s, f);
                if (pos < 0) {
                    ret = pos;
                    goto fail;
                }
                ppos[x] = pos;

                for (int i = 0; i < nc; i++) {
                    const int qerr = px[i] - s->idx2val[f[i]];

                    err_cur [(x + dir) * nc + i] += qerr * 7;
                    err_next[(x - dir) * nc + i] += qerr * 3;
                    err_next[ x        * nc + i] += qerr * 5;
                    err_next[(x + dir) * nc + i] += qerr;
                }
            }
        } else if (s->dither == DITHERING_BAYER || s->dither == DITHERING_BLUE_NOISE) {
            const int blue = s->dither == DITHERING_BLUE_NOISE;
            const int mask = blue ? VC_MASK : 7;
            const int rowoff = blue ? (y & VC_MASK) << VC_SHIFT : (y & 7) << 3;
            const int *dt[4];

            for (int i = 0; i < nc; i++)
                dt[i] = (blue ? s->blue_dither[i] : s->ordered_dither[i]) + rowoff;

            for (int x = 0; x < w; x++) {
                px[0] = src[x * pixstep + ro] + dt[0][x & mask];
                px[1] = src[x * pixstep + go] + dt[1][x & mask];
                px[2] = src[x * pixstep + bo] + dt[2][x & mask];
                if (nc == 4)
                    px[3] = (ao >= 0 ? src[x * pixstep + ao] : 255) + dt[3][x & mask];

                quant_dn(s, px, f);
                pos = color_inc(s, f);
                if (pos < 0) {
                    ret = pos;
                    goto fail;
                }
                ppos[x] = pos;
            }
        } else {
            for (int x = 0; x < w; x++) {
                px[0] = src[x * pixstep + ro];
                px[1] = src[x * pixstep + go];
                px[2] = src[x * pixstep + bo];
                if (nc == 4)
                    px[3] = ao >= 0 ? src[x * pixstep + ao] : 255;

                quant_dn(s, px, f);
                pos = color_inc(s, f);
                if (pos < 0) {
                    ret = pos;
                    goto fail;
                }
                ppos[x] = pos;
            }
        }
    }

    reduced = s->nb_used > s->max_colors;
    if (reduced) {
        if (s->refine == REFINE_BATCHED)
            ret = reduce_batched(s, in);
        else
            ret = reduce_colors(s, s->max_colors);
        if (ret < 0)
            goto fail;
        av_log(ctx, AV_LOG_DEBUG,
               "pts %"PRId64": %d lattice colors used, %d dropped\n",
               in->pts, s->nb_used, s->nb_used - s->nb_alive);
    }

    /* Assign palette slots so that each entry is identical or similar to
     * the same entry of the previous frame: static regions keep their
     * indices and both the index plane and the palette stay small under
     * inter prediction. Unclaimed slots keep the previous frame's color.
     * On the first frame the all black previous palette degenerates this
     * to insertion order. */
    {
        uint8_t claimed[AVPALETTE_COUNT] = { 0 };

        for (int i = 0; i < s->nb_used; i++) {
            PalEntry *e = &s->list[i];
            uint32_t c;

            if (!e->alive)
                continue;
            e->nn = -1;
            c = entry_color(s, e);
            for (int slot = 0; slot < AVPALETTE_COUNT; slot++) {
                if (!claimed[slot] && s->prev_pal[slot] == c) {
                    claimed[slot] = 1;
                    e->nn = slot;
                    break;
                }
            }
        }
        for (int i = 0; i < s->nb_used; i++) {
            PalEntry *e = &s->list[i];
            uint32_t c;
            int bslot = -1;
            int64_t bd = INT64_MAX;

            if (!e->alive || e->nn >= 0)
                continue;
            c = entry_color(s, e);
            for (int slot = 0; slot < AVPALETTE_COUNT; slot++) {
                const uint32_t p = s->prev_pal[slot];
                const int da = (int)(p >> 24        ) - (int)(c >> 24        );
                const int dr = (int)(p >> 16 & 0xff) - (int)(c >> 16 & 0xff);
                const int dg = (int)(p >>  8 & 0xff) - (int)(c >>  8 & 0xff);
                const int db = (int)(p       & 0xff) - (int)(c       & 0xff);
                const int64_t d2 = (int64_t)da * da + dr * dr + dg * dg + db * db;

                if (!claimed[slot] && d2 < bd) {
                    bd    = d2;
                    bslot = slot;
                }
            }
            claimed[bslot] = 1;
            e->nn = bslot;
        }

        pal = (uint32_t *)out->data[1];
        memcpy(pal, s->prev_pal, AVPALETTE_SIZE);
        for (int i = 0; i < s->nb_used; i++) {
            const PalEntry *e = &s->list[i];

            if (e->alive)
                pal[e->nn] = entry_color(s, e);
        }
        memcpy(s->prev_pal, pal, AVPALETTE_SIZE);
    }
    if (!(s->refine == REFINE_BATCHED && reduced)) {
        for (int i = 0; i < s->nb_used; i++) {
            PalEntry *e = &s->list[i];

            if (!e->alive) {
                nn_search(s, i);
                e->nn = s->list[e->nn].nn;
            }
        }
    }

    if (s->refine == REFINE_BATCHED && reduced) {
        /* pixpos holds the list index assigned by the last rediffusion */
        for (int y = 0; y < h; y++) {
            const uint32_t *ppos = s->pixpos + (size_t)y * w;
            uint8_t *dst = out->data[0] + y * out->linesize[0];

            for (int x = 0; x < w; x++)
                dst[x] = s->list[ppos[x]].nn;
        }
    } else if (s->refine == REFINE_RESIDUAL && reduced) {
        /* needs the cell table still holding list indices */
        ret = refine_residual(s, out, in);
        if (ret < 0)
            goto fail;
    } else if (s->refine == REFINE_FULL && reduced) {
        /* rewrite the cell table to slot + 1, 0 keeps meaning unused */
        for (int i = 0; i < s->nb_used; i++)
            s->cell[s->list[i].pos] = s->list[i].nn + 1;
        ret = refine_full(s, out, in);
        if (ret < 0)
            goto fail;
    } else {
        /* second pass: write the palette indices */
        for (int i = 0; i < s->nb_used; i++)
            s->cell[s->list[i].pos] = s->list[i].nn;
        for (int y = 0; y < h; y++) {
            const uint32_t *ppos = s->pixpos + (size_t)y * w;
            uint8_t *dst = out->data[0] + y * out->linesize[0];

            for (int x = 0; x < w; x++)
                dst[x] = s->cell[ppos[x]];
        }
    }

    /* reset the cell table for the next frame */
    for (int i = 0; i < s->nb_used; i++)
        s->cell[s->list[i].pos] = 0;
    for (int i = 0; i < s->nb_touched; i++)
        s->cell[s->touched[i]] = 0;
    s->nb_touched = 0;

    av_frame_free(&in);
    return ff_filter_frame(outlink, out);

fail:
    for (int i = 0; i < s->nb_used; i++)
        s->cell[s->list[i].pos] = 0;
    for (int i = 0; i < s->nb_touched; i++)
        s->cell[s->touched[i]] = 0;
    s->nb_touched = 0;
    av_frame_free(&in);
    av_frame_free(&out);
    return ret;
}

static int config_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    LatticePalContext *s = ctx->priv;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);
    const int N = s->density;

    s->pixstep = desc->comp[0].step;
    s->ro = desc->comp[0].offset;
    s->go = desc->comp[1].offset;
    s->bo = desc->comp[2].offset;
    s->ao = desc->nb_components == 4 && (desc->flags & AV_PIX_FMT_FLAG_ALPHA)
            ? desc->comp[3].offset : -1;
    s->nc = s->alpha ? 4 : 3;

    if (s->alpha && N > MAX_DENSITY_ALPHA) {
        av_log(ctx, AV_LOG_ERROR,
               "density is limited to %d when the alpha channel is quantized\n",
               MAX_DENSITY_ALPHA);
        return AVERROR(EINVAL);
    }

    s->scale = N / 255.f;
    for (int i = 0; i <= N; i++)
        s->idx2val[i] = lrintf(i * 255.f / N);

    for (int i = 0; i < AVPALETTE_COUNT; i++)
        s->prev_pal[i] = 0xFF000000;

    s->dim = N + 1;
    s->min_gap = 255;
    for (int i = 1; i <= N; i++)
        s->min_gap = FFMIN(s->min_gap, s->idx2val[i] - s->idx2val[i - 1]);

    av_freep(&s->cell);
    s->cell = av_calloc(s->nc == 4 ? (size_t)s->dim * s->dim * s->dim * s->dim
                                   : (size_t)s->dim * s->dim * s->dim,
                        sizeof(*s->cell));
    av_freep(&s->pixpos);
    s->pixpos = av_malloc_array((size_t)inlink->w * inlink->h, sizeof(*s->pixpos));
    if (!s->cell || !s->pixpos)
        return AVERROR(ENOMEM);

    if (s->dither == DITHERING_BAYER) {
        /* The per-channel period of the scaled D3 lattice is 2*step, not
         * step: translating the lattice by step*e_i flips the parity
         * constraint, only 2*step*e_i maps it onto itself. The dither
         * offsets must span one period for intermediate colors to be
         * reproduced on average, hence the amplitude of 2*step.
         *
         * The matrix and the per-channel decorrelation follow the classic
         * swscale ordered dither (libswscale/yuv2rgb.c): all channels use
         * ff_dither_8x8_73, green shifted by one column and blue with
         * flipped rows. The channel patterns are strongly anti-correlated
         * (-0.94 R/G, -0.60 R/B), which keeps the offset sum and with it
         * the luminance noise small, and it interacts well with the D3
         * quantizer: over a 40..215 color cube this scheme measures the
         * smallest flat field bias (max 8.3, mean 3.5) and the smallest
         * luminance RMS noise (15.3) of all evaluated 8x8 schemes. With
         * alpha, the fourth channel combines both transforms. */
        const float amp = 2.f * 255.f / N;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                const int i = y << 3 | x;
                s->ordered_dither[0][i] = lrintf(((dither_8x8_73[y][x]               + 0.5f) / 64.f - 0.5f) * amp);
                s->ordered_dither[1][i] = lrintf(((dither_8x8_73[y][(x + 1) & 7]     + 0.5f) / 64.f - 0.5f) * amp);
                s->ordered_dither[2][i] = lrintf(((dither_8x8_73[y ^ 7][x]           + 0.5f) / 64.f - 0.5f) * amp);
                s->ordered_dither[3][i] = lrintf(((dither_8x8_73[y ^ 7][(x + 1) & 7] + 0.5f) / 64.f - 0.5f) * amp);
            }
        }
    } else if (s->dither == DITHERING_BLUE_NOISE) {
        /* Dither in a lattice adapted frame: step*(1,1,0), step*(1,-1,0)
         * and 2*step*(0,0,1) span an orthogonal sublattice of the color
         * lattice, so offsets drawn uniformly from its fundamental box tile
         * the color space under lattice translations, and since Dn Voronoi
         * cells are centrally symmetric the dithered average is exactly the
         * input color. Independently seeded void-and-cluster masks
         * a, b, c (and d with alpha) drive the axes:
         *     tR = step * (a + b - 1)
         *     tG = step * (a - b)
         *     tB = step * (2c - 1)
         * Compared with independent per-channel offsets over the axis
         * aligned (-step,step)^3 box (index 4 instead of 2) this cuts the
         * red/green noise variance in half; the remaining full range axis
         * is assigned to blue, the perceptually least weighted channel.
         * For D4 the sublattice step*{(1,1,0,0), (1,-1,0,0), (0,0,1,1),
         * (0,0,1,-1)} is orthogonal with equally long axes, so all four
         * channels get the halved variance:
         *     tB = step * (c + d - 1)
         *     tA = step * (c - d) */
        const float step = 255.f / N;
        float kern[VC_KSIZE * VC_KSIZE];
        uint16_t *ranks[4] = { NULL };
        uint8_t  *bits = NULL;
        float *energy  = NULL, *e1 = NULL;
        int ret = 0;

        for (int dy = -VC_RADIUS; dy <= VC_RADIUS; dy++)
            for (int dx = -VC_RADIUS; dx <= VC_RADIUS; dx++)
                kern[(dy + VC_RADIUS) * VC_KSIZE + dx + VC_RADIUS] =
                    expf(-(dx * dx + dy * dy) / (2 * VC_SIGMA * VC_SIGMA));

        bits   = av_malloc_array(VC_AREA, sizeof(*bits));
        energy = av_malloc_array(VC_AREA, sizeof(*energy));
        e1     = av_malloc_array(VC_AREA, sizeof(*e1));
        ret = !bits || !energy || !e1 ? AVERROR(ENOMEM) : 0;

        for (int p = 0; p < s->nc && ret >= 0; p++) {
            AVLFG lfg;

            ranks[p] = av_malloc_array(VC_AREA, sizeof(*ranks[p]));
            if (!s->blue_dither[p])
                s->blue_dither[p] = av_malloc_array(VC_AREA, sizeof(*s->blue_dither[p]));
            if (!ranks[p] || !s->blue_dither[p]) {
                ret = AVERROR(ENOMEM);
                break;
            }
            av_lfg_init(&lfg, 0xB1DE + p);
            vc_generate(ranks[p], bits, energy, e1, kern, &lfg);
        }

        if (ret >= 0) {
            for (int i = 0; i < VC_AREA; i++) {
                const float a = (ranks[0][i] + 0.5f) / VC_AREA;
                const float b = (ranks[1][i] + 0.5f) / VC_AREA;
                const float c = (ranks[2][i] + 0.5f) / VC_AREA;

                s->blue_dither[0][i] = lrintf(step * (a + b - 1.f));
                s->blue_dither[1][i] = lrintf(step * (a - b));
                if (s->nc == 4) {
                    const float dd = (ranks[3][i] + 0.5f) / VC_AREA;

                    s->blue_dither[2][i] = lrintf(step * (c + dd - 1.f));
                    s->blue_dither[3][i] = lrintf(step * (c - dd));
                } else {
                    s->blue_dither[2][i] = lrintf(step * (2.f * c - 1.f));
                }
            }
        }

        av_freep(&ranks[0]);
        av_freep(&ranks[1]);
        av_freep(&ranks[2]);
        av_freep(&ranks[3]);
        av_freep(&bits);
        av_freep(&energy);
        av_freep(&e1);
        if (ret < 0)
            return ret;
    }
    if (s->dither == DITHERING_FLOYD_STEINBERG || s->refine) {
        for (int i = 0; i < 2; i++) {
            av_freep(&s->err[i]);
            s->err[i] = av_calloc(inlink->w + 2, s->nc * sizeof(*s->err[i]));
            if (!s->err[i])
                return AVERROR(ENOMEM);
        }
    }

    return 0;
}

static av_cold void uninit(AVFilterContext *ctx)
{
    LatticePalContext *s = ctx->priv;

    av_freep(&s->err[0]);
    av_freep(&s->err[1]);
    for (int i = 0; i < 4; i++)
        av_freep(&s->blue_dither[i]);
    av_freep(&s->cell);
    av_freep(&s->pixpos);
    av_freep(&s->list);
    av_freep(&s->alive_arr);
    av_freep(&s->alive_pos);
    av_freep(&s->touched);
}

static const AVFilterPad latticepal_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
        .config_props = config_input,
    },
};

const FFFilter ff_vf_latticepal = {
    .p.name        = "latticepal",
    .p.description = NULL_IF_CONFIG_SMALL("Convert RGB to PAL8 using a per-frame FCC lattice palette."),
    .p.priv_class  = &latticepal_class,
    .priv_size     = sizeof(LatticePalContext),
    .uninit        = uninit,
    FILTER_INPUTS(latticepal_inputs),
    FILTER_OUTPUTS(ff_video_default_filterpad),
    FILTER_QUERY_FUNC2(query_formats),
};
