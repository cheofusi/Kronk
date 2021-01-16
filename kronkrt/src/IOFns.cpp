#include <iostream>
#include <cstdarg>

// Input Output functions
extern "C" {
    
    void afficher(const char* fmt, ...) {
        bool val_bool;
        double val_reel;
        const char* chaine_car;

        std::va_list args;
        va_start(args, fmt);
    
        while (*fmt != '\0') {
            switch (*fmt) {
                case 'b':
                    val_bool = va_arg(args, int);
                    std::cout << val_bool << ' ';
                    break;

                case 'r':
                    val_reel = va_arg(args, double);
                    std::cout << val_reel << ' '; 
                    break;
                
                case 's':
                    chaine_car = va_arg(args, const char*);
                    std::cout << chaine_car << ' ';   
            }

            ++fmt;
        }

        std::cout << '\n';
        va_end(args);
    }
}


 