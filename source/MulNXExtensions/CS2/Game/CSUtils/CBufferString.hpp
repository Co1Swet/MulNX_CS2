#pragma once

namespace CS2 {
    class CBufferString {
        int length;
        int allocSizeWithFlag;
        union {
            char* pHeapString;
            char internalString[8];
        };
    public:
        using Insert_t = const char* (*)(void*, int nIndex, const char* pBuf, int nCount, bool bIgnoreAlignment);
        inline static Insert_t pFuncInsert = nullptr;

        using Purge_t = const void(*)(void*, int nAllocatedBytesToPreserve);
        inline static Purge_t pFuncPurge = nullptr;

        using FixupPathName_t = const char* (*)(void*, char);
        inline static FixupPathName_t pFuncFixupPathName = nullptr;

        using ToLowerFast_t = const char* (*)(void*, int);
        inline static ToLowerFast_t pFuncToLowerFast = nullptr;

        using FixSlashes_t = const char* (*)(void*, char);
        inline static FixSlashes_t pFuncFixSlashes = nullptr;

        using ExtractFileExtension_t = const char* (*)(void*, const char*);
        inline static ExtractFileExtension_t pFuncExtractFileExtension = nullptr;

        CBufferString(bool bAllowHeapAllocation = true) :
            length(0),
            allocSizeWithFlag((bAllowHeapAllocation* (1 << 31)) | (1 << 30) | 8),
            pHeapString(nullptr) {}

        CBufferString(const char* pString, bool bAllowHeapAllocation = true) :
            CBufferString(bAllowHeapAllocation) {
            this->Insert(0, pString);
        }

        CBufferString(const CBufferString& other) = delete;
        CBufferString& operator=(const CBufferString& src) = delete;

        ~CBufferString() { this->Purge(0); }
        
        void Insert(int nIndex, const char* pBuf, int nCount = -1, bool bIgnoreAlignment = false) {
            this->pFuncInsert(this, nIndex, pBuf, nCount, bIgnoreAlignment);
        }
        void Purge(int nAllocatedBytesToPreserve) {
            this->pFuncPurge(this, nAllocatedBytesToPreserve);
        }
        void FixupPathName(char cSeparator) {
            this->pFuncFixupPathName(this, cSeparator);
        }
        void ToLowerFast(int nStart) {
            this->pFuncToLowerFast(this, nStart);
        }
        void FixSlashes(char cSeparator) {
            this->pFuncFixSlashes(this, cSeparator);
        }
        void ExtractFileExtension(const char* pPath) {
            this->pFuncExtractFileExtension(this, pPath);
        }
    };
}