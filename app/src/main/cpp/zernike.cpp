#include "zernike.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace std;

#define MAX_L 32
#define MAX_D 15
#define MAX_Z 72
#define MAX_LUT 240

// Implementación del factorial sin GSL
long double factorial(int n) {
    if (n <= 1) return 1.0;
    long double result = 1.0;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

void mb_Znl(double *X, double *Y, double *P, int size, double D, double m10_m00, double m01_m00, double R, double psum, double *zvalues, long *output_size) {
    static double LUT[MAX_LUT];
    static int n_s[MAX_Z], l_s[MAX_Z];
    static char init_lut = 0;

    double x, y, p;
    int i, m, theZ, theLUT, numZ = 0;
    int n = 0, l = 0;

    complex<double> sum[MAX_Z];
    complex<double> Vnl;

    assert(D == MAX_D);

    if (!init_lut) {
        theZ = 0;
        theLUT = 0;
        for (n = 0; n <= MAX_D; n++) {
            for (l = 0; l <= n; l++) {
                if ((n - l) % 2 == 0) {
                    for (m = 0; m <= (n - l) / 2; m++) {
                        LUT[theLUT] = pow(-1.0, m) * (factorial(n - m) / (factorial(m) * factorial((n - 2 * m + l) / 2) * factorial((n - 2 * m - l) / 2)));
                        theLUT++;
                    }
                    n_s[theZ] = n;
                    l_s[theZ] = l;
                    theZ++;
                }
            }
        }
        init_lut = 1;
    }

    for (n = 0; n <= D; n++) {
        for (l = 0; l <= n; l++) {
            if ((n - l) % 2 == 0) {
                sum[numZ] = complex<double>(0.0, 0.0);
                numZ++;
            }
        }
    }

    for (i = 0; i < size; i++) {
        x = (X[i] - m10_m00) / R;
        y = (Y[i] - m01_m00) / R;
        double sqr_x2y2 = sqrt(x * x + y * y);
        if (sqr_x2y2 > 1.0) continue;

        p = P[i] / psum;
        double atan2yx = atan2(y, x);
        theLUT = 0;
        for (theZ = 0; theZ < numZ; theZ++) {
            n = n_s[theZ];
            l = l_s[theZ];
            Vnl = complex<double>(0.0, 0.0);
            for (m = 0; m <= (n - l) / 2; m++) {
                Vnl += (polar(1.0, l * atan2yx) * LUT[theLUT] * pow(sqr_x2y2, (double)(n - 2 * m)));
                theLUT++;
            }
            sum[theZ] += (conj(Vnl) * p);
        }
    }

    double preal, pimag;
    for (theZ = 0; theZ < numZ; theZ++) {
        sum[theZ] *= ((n_s[theZ] + 1) / PI);
        preal = real(sum[theZ]);
        pimag = imag(sum[theZ]);
        zvalues[theZ] = fabs(sqrt(preal * preal + pimag * pimag));
    }

    *output_size = numZ;
}

void mb_zernike2D(double *image, int width, int height, double order, double rad, double *zvalues, long *output_size) {
    vector<double> X, Y, P;
    double psum = 0.0;

    int size = 0;
    double moment10 = 0.0, moment00 = 0.0, moment01 = 0.0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double intensity = image[y * width + x];
            if (intensity != 0) {
                X.push_back(x + 1);
                Y.push_back(y + 1);
                P.push_back(intensity);
                psum += intensity;
                size++;
            }
            moment10 += (x + 1) * intensity;
            moment00 += intensity;
            moment01 += (y + 1) * intensity;
        }
    }

    double m10_m00 = moment10 / moment00;
    double m01_m00 = moment01 / moment00;

    mb_Znl(X.data(), Y.data(), P.data(), size, order, m10_m00, m01_m00, rad, psum, zvalues, output_size);
}
