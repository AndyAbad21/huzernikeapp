#ifndef ZERNIKE_H
#define ZERNIKE_H

#include <complex>
#include <cmath>
#include <vector>

// Definir PI si no está definido
#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

// Factorial sin GSL
long double factorial(int n);

// Función principal para calcular momentos de Zernike
void mb_Znl(double *X, double *Y, double *P, int size, double D, double m10_m00, double m01_m00, double R, double psum, double *zvalues, long *output_size);

// Implementación de Zernike en 2D (sin dependencia de ImageMatrix)
void mb_zernike2D(double *image, int width, int height, double order, double rad, double *zvalues, long *output_size);

#endif // ZERNIKE_H
