//#include <cairo/cairo.h>
//#include <math.h>

extern int CanvasColorR; // draw param declared as global in ir.ll. 
extern int CanvasColorG;
extern int CanvasColorB;

extern int CanvasWidth;
extern int CanvasHeight;

int draw(int i){
    return CanvasHeight * CanvasWidth * CanvasColorG; // if this works test draw functions
}

// int main(void) {  
//     cairo_surface_t *surface;
//     cairo_t *cr;

//     surface =  cairo_image_surface_create(CAIRO_FORMAT_RGB24, 3508, 2480);
//     cr = cairo_create(surface);

//     cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
//     cairo_paint(cr);
//     cairo_translate(cr, 3508/2, 2480/2);
//     cairo_set_source_rgb(cr, 1.0, 0.6, 0.6);
//     cairo_set_line_width(cr, 1);

//     cairo_rectangle(cr, 20, 20, 120, 80);
//     cairo_rectangle(cr, 180, 20, 80, 80);
//     cairo_stroke_preserve(cr);
//     cairo_fill(cr);

//     cairo_arc(cr, 330, 60, 40, 0, 2 * M_PI);
//     cairo_stroke_preserve(cr);
//     cairo_fill(cr);

//     cairo_arc(cr, 90, 160, 40, M_PI/4, M_PI);
//     cairo_close_path(cr);
//     cairo_stroke_preserve(cr);
//     cairo_fill(cr);

//     cairo_translate(cr, 220, 180);
//     cairo_scale(cr, 1, 0.7);
//     cairo_arc(cr, 0, 0, 50, 0, 2*M_PI);
//     cairo_stroke_preserve(cr);
//     cairo_fill(cr);

//     cairo_surface_write_to_png(surface, "image.png");

//     cairo_destroy(cr);
//     cairo_surface_destroy(surface);

//     return 0;
// }