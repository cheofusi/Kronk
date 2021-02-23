#include <stdlib.h>
#include <stdio.h>


// These are functions that may be used by kronkc during compilation. In the future they'll be 
// translated to kronk

extern "C" {

bool _kronk_idx_check(int64_t idx, int64_t listSize) {
    return (idx >= 0) ? (idx < listSize) : (labs(idx) <= listSize);
}


void _kronk_list_idx_check(int64_t idx, int64_t listSize, const char* file, int64_t lineNumber) {
    bool res = _kronk_idx_check(idx, listSize);

    if(not res) {
        printf("%s%s%s\n%s%d%s%d\n",
                "In ",
                file,
                ",", 
                "[Line ", 
                lineNumber, 
                "]: IndexError: while indexing liste of size ",
                listSize);
        
        exit(EXIT_FAILURE); 
    }
}


void _kronk_list_slice_check(int64_t start, int64_t end,  int64_t listSize, const char* file, int64_t lineNumber) {

    bool res = (start > end) ? false :  ( _kronk_idx_check(start, listSize) || (start == listSize) )
                                    &&  ( _kronk_idx_check(end, listSize) || (end == listSize) );

    if(not res) {
        printf("%s%s%s\n%s%d%s%d\n",
                "In ",
                file,
                ",", 
                "[Line ", 
                lineNumber, 
                "]: SliceError: while slicing liste of size ",
                listSize);
        
        exit(EXIT_FAILURE); 
    }
}


int64_t _kronk_list_fix_idx(int64_t idx, int64_t listSize) {
    return (idx < 0) ? idx + listSize : idx;
}


void _kronk_zero_div_check(double x, const char* file, int64_t lineNumber) {
    if(x == 0) {
        printf("%s%s%s\n%s%d%s\n",
                "In ",
                file,
                ",", 
                "[Line ", 
                lineNumber, 
                "]: ZeroDivisionError ");
        
        exit(EXIT_FAILURE);
    }
}   

}