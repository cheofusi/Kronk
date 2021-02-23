#include "math.h"

extern "C" {

///////////////////  Algebraic functioins ///////////////////////
double _kmath_puiss(double x, double y) {
    return pow(x, y);
}

double _kmath_exp(double x) {
    return exp(x);
}

double _kmath_mod(double x, double y) {
    return fmod(x, y);
}
/////////////////////////////////////////////////////////////////


///////////////////  Trigonometric functions ////////////////////
double _kmath_sin(double x) {
    return sin(x);
}

double _kmath_cos(double x) {
    return cos(x);
}

double _kmath_tan(double x) {
    return tan(x);
}
/////////////////////////////////////////////////////////////////

}