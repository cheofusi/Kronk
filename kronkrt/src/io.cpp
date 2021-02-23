#include <stdio.h>
#include <stdlib.h>
#include <cstdarg>

// Input Output functions
extern "C" {
    
    void _kio_afficher(const char* fmt, ...) {
        bool val_bool;
        double val_reel;
        const char* str;
        int64_t str_size;

        va_list args;
        va_start(args, fmt);
    
        while (*fmt != '\0') {
            switch (*fmt) {
                case 'b':
                    val_bool = va_arg(args, int);
                    printf("%s ", (val_bool ? "vrai" : "faux"));
                    break;

                case 'r':
                    val_reel = va_arg(args, double);
                    printf("%g ", val_reel); 
                    break;
                
                case 's':
                    str = va_arg(args, const char*);
                    if(*(fmt + 1) == 'd') {
                        str_size = va_arg(args, int64_t);
                        for(int64_t i=0; i < str_size; ++i)
                            printf("%c", str[i]);

                        printf(" ");
                        ++fmt; 
                    }

                    else {
                        printf("%s ", str);   
                    }

            }

            ++fmt;
        }

        printf("\n");
        va_end(args);
    }
}


 