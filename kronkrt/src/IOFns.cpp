#include <iostream>
#include <cstdarg>

// Input Output functions
extern "C" {
    
    void afficher(const char* fmt, ...) {
        bool val_bool;
        double val_reel;
        const char* str;
        int64_t str_size;

        std::va_list args;
        va_start(args, fmt);
    
        while (*fmt != '\0') {
            switch (*fmt) {
                case 'b':
                    val_bool = va_arg(args, int);
                    std::cout << (val_bool ? "vrai" : "faux") << ' ';
                    break;

                case 'r':
                    val_reel = va_arg(args, double);
                    std::cout << val_reel << ' '; 
                    break;
                
                case 's':
                    str = va_arg(args, const char*);
                    if(*(fmt + 1) == 'd') {
                        str_size = va_arg(args, int64_t);
                        for(int64_t i=0; i < str_size; ++i)
                            std::cout << str[i];

                        std::cout <<' ';
                        ++fmt; 
                    }

                    else {
                        std::cout << str << ' ';   
                    }

            }

            ++fmt;
        }

        std::cout << '\n';
        va_end(args);
    }
}


 