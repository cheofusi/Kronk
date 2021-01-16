#include <stdlib.h>
#include <stdio.h>
#include <memory.h>


/// These are functions that may be used by kronkc during compilation 
extern "C" {

    int _kronk_runtime_error(const char* errMsg) {
        printf("%s\n", errMsg);
        exit(1);
    }


    void _kronk_memcpy(void* src, void* dest, int64_t num_bytes){
        memcpy(dest, src, num_bytes);
    }


    bool _kronk_list_index_check(int64_t idx, int64_t listSize) {
        if(idx >= 0){
            return (idx < listSize) ? 1 : 0;
        }
        return (labs(idx) <= listSize) ? 1 : 0;
    }


    bool _kronk_list_splice_check(int64_t start, int64_t end,  int64_t listSize) {
        if(start > end){
            return 0;
        }
        return     ( _kronk_list_index_check(start, listSize) || (start == listSize) )
                && ( _kronk_list_index_check(end, listSize) || (end == listSize) );
    }


    int64_t _kronk_list_fix_idx(int64_t idx, int64_t listSize) {
        return (idx < 0) ? idx + listSize : idx;
    }

}