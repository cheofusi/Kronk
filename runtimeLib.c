#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

int _kronk_runtime_error(const char* errMsg) {
    printf("%s\n", errMsg);
    exit(1);
}


void _kronk_memcpy(void* src, void* dest, __int64_t num_bytes){
    memcpy(dest, src, num_bytes);
}


_Bool _kronk_list_index_check(__int64_t idx, __int64_t listSize) {
    if(idx >= 0){
        return (idx < listSize) ? 1 : 0;
    }
    return (labs(idx) <= listSize) ? 1 : 0;
}

_Bool _kronk_list_splice_check(__int64_t start, __int64_t end,  __int64_t listSize) {
    if(start > end){
        return 0;
    }
    return     ( _kronk_list_index_check(start, listSize) || (start == listSize) )
            && ( _kronk_list_index_check(end, listSize) || (end == listSize) );
}

__int64_t _kronk_list_fix_idx(__int64_t idx, __int64_t listSize) {
    return (idx < 0) ? idx + listSize : idx;
}
