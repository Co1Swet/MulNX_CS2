#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
from pathlib import Path
import time
from collections import defaultdict

# ---------- 树构建 ----------
def _make_node():
    return {"children": {}, "lines": 0, "is_file": False}

def build_tree(files_info):
    """files_info: [(相对路径字符串, 行数), ...] -> 树根节点"""
    root = _make_node()
    for rel, lines in files_info:
        parts = Path(rel).parts
        node = root
        node["lines"] += lines
        for part in parts[:-1]:
            child = node["children"].setdefault(part, _make_node())
            child["lines"] += lines
            node = child
        leaf = _make_node()
        leaf["lines"] = lines
        leaf["is_file"] = True
        node["children"][parts[-1]] = leaf
    return root

def render_tree(node, prefix="", out=None):
    """递归渲染树，返回行列表"""
    if out is None:
        out = []
    # 目录优先，文件其次；同类按名称排序
    items = sorted(
        node["children"].items(),
        key=lambda kv: (kv[1]["is_file"], kv[0].lower())
    )
    for i, (name, child) in enumerate(items):
        is_last = (i == len(items) - 1)
        conn = "└── " if is_last else "├── "
        suffix = "" if child["is_file"] else "/"
        out.append(f"{prefix}{conn}{name}{suffix}  ({child['lines']:,} 行)")
        if not child["is_file"]:
            child_prefix = prefix + ("    " if is_last else "│   ")
            render_tree(child, child_prefix, out)
    return out

# ---------- 主统计 ----------
def count_cpp_lines():
    current_dir = Path.cwd()
    parent_dir = current_dir.parent

    main_extensions = ('.cpp', '.hpp')
    third_party_extensions = ('.cpp', '.hpp', '.h', '.c')
    excluded_keyword = ('MulNXThirdParty', 'ThirdParty', 'build', 'MulNXDrop')

    total_lines = 0
    total_files = 0
    excluded_total_lines = 0
    excluded_file_count = 0

    processed_files = []       # 主项目 (相对路径, 行数)
    excluded_files_info = []   # 第三方 (相对路径, 行数)
    error_files = []

    print(f"当前工作目录: {current_dir}")
    print(f"统计目标目录: {parent_dir}")
    print(f"主项目统计扩展名: {', '.join(main_extensions)}")
    print(f"第三方统计扩展名: {', '.join(third_party_extensions)}")
    print(f"排除关键字: {', '.join(excluded_keyword)}")
    print(f"忽略目录: FFmpegBuild")
    print(f"{'='*60}")
    print("开始扫描目录...")

    start_time = time.time()
    dir_count = 0
    scanned_count = 0

    for root, dirs, files in os.walk(parent_dir):
        dirs[:] = [d for d in dirs if 'FFmpegBuild' not in d]
        dir_count += 1

        all_files = [f for f in files if f.endswith(main_extensions + third_party_extensions)]

        for file in all_files:
            file_path = Path(root) / file
            file_path_str = str(file_path)

            is_third_party = any(kw in file_path_str for kw in excluded_keyword)

            if is_third_party:
                if not file.endswith(third_party_extensions):
                    continue
            else:
                if not file.endswith(main_extensions):
                    continue

            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    line_count = sum(1 for _ in f)

                relative_path = file_path.relative_to(parent_dir)

                if is_third_party:
                    excluded_total_lines += line_count
                    excluded_file_count += 1
                    excluded_files_info.append((str(relative_path), line_count))
                else:
                    total_lines += line_count
                    total_files += 1
                    processed_files.append((str(relative_path), line_count))

                scanned_count += 1
                if scanned_count % 100 == 0:
                    print(".", end="", flush=True)

            except PermissionError:
                error_files.append((str(file_path.relative_to(parent_dir)), "权限不足"))
            except Exception as e:
                error_files.append((str(file_path.relative_to(parent_dir)), str(e)))

    print()
    elapsed_time = time.time() - start_time
    return (total_lines, total_files, parent_dir, processed_files,
            excluded_total_lines, excluded_file_count, excluded_files_info,
            error_files, dir_count, elapsed_time)

def main():
    print("=" * 60)
    print("C++ 代码行数统计工具 - 树状输出")
    print("=" * 60)

    try:
        (total_lines, total_files, parent_dir, processed_files,
         excluded_total_lines, excluded_file_count, excluded_files_info,
         error_files, dir_count, elapsed_time) = count_cpp_lines()

        print(f"\n{'='*60}")
        print("统计摘要")
        print(f"{'='*60}")
        print(f"统计目录          : {parent_dir}")
        print(f"扫描目录数量      : {dir_count}")
        print(f"扫描耗时          : {elapsed_time:.2f} 秒")
        print(f"{'─'*60}")
        print(f"【主项目代码】")
        print(f"  文件总数        : {total_files}")
        print(f"  总代码行数      : {total_lines:,}")
        if total_files > 0:
            print(f"  平均每文件行数  : {total_lines / total_files:,.2f}")

        # ---- 主项目：树状展开 ----
        if processed_files:
            tree_root = build_tree(processed_files)
            print(f"\n{'─'*60}")
            print("主项目代码树（目录后数字为该目录下所有代码行数之和）:")
            print(f"{'─'*60}")
            for line in render_tree(tree_root):
                print(line)

        # ---- 第三方：只按顶层目录汇总 ----
        print(f"\n{'─'*60}")
        print(f"【第三方代码（不纳入主代码总量，仅汇总）】")
        print(f"  文件总数        : {excluded_file_count}")
        print(f"  总代码行数      : {excluded_total_lines:,}")
        if excluded_files_info:
            groups = defaultdict(lambda: [0, 0])  # key -> [lines, count]
            for rel, lines in excluded_files_info:
                parts = Path(rel).parts
                key = "/".join(parts[:2]) if len(parts) >= 2 else parts[0]
                groups[key][0] += lines
                groups[key][1] += 1
            print(f"  按来源汇总:")
            for key, (lines, cnt) in sorted(groups.items(), key=lambda kv: kv[1][0], reverse=True):
                print(f"    {key:<45} : {lines:>10,} 行 ({cnt} 文件)")

        print(f"{'─'*60}")
        print(f"读取错误文件数    : {len(error_files)}")
        if error_files:
            for i, (fp, msg) in enumerate(error_files[:10]):
                print(f"  {i+1:2d}. {fp}")
                print(f"      错误: {msg}")
            if len(error_files) > 10:
                print(f"  ... 以及 {len(error_files) - 10} 个其他错误")

        print(f"\n{'='*60}")
        print("统计完成！")
        print(f"{'='*60}")
        input("\n按 Enter 键退出...")

    except KeyboardInterrupt:
        print("\n\n用户中断操作")
    except Exception as e:
        print(f"\n发生错误: {e}")
        import traceback
        traceback.print_exc()
        input("\n按 Enter 键退出...")

if __name__ == "__main__":
    main()