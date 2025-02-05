#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <android/bitmap.h>
#include <android/log.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_TAG "JNI_DEBUG"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace cv;

// Ruta de almacenamiento en el teléfono
const string STORAGE_PATH = "/storage/emulated/0/Documents/Test/";

// Función para crear la carpeta si no existe
void createDirectoryIfNeeded(const string &path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        mkdir(path.c_str(), 0777);
    }
}

Mat bitmapToMat(JNIEnv *env, jobject bitmap) {
    AndroidBitmapInfo bitmapInfo;
    void *pixels = nullptr;

    if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0) {
        throw runtime_error("Error al obtener información del Bitmap");
    }

    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        throw runtime_error("Error al bloquear los píxeles del Bitmap");
    }

    Mat mat(bitmapInfo.height, bitmapInfo.width, CV_8UC3);
    uint16_t *src = static_cast<uint16_t *>(pixels);
    for (int y = 0; y < bitmapInfo.height; y++) {
        Vec3b *row = mat.ptr<Vec3b>(y);
        for (int x = 0; x < bitmapInfo.width; x++) {
            uint16_t pixel = src[y * bitmapInfo.width + x];
            uint8_t r = (pixel & 0xF800) >> 8;
            uint8_t g = (pixel & 0x07E0) >> 3;
            uint8_t b = (pixel & 0x001F) << 3;
            row[x] = Vec3b(b, g, r);
        }
    }
    AndroidBitmap_unlockPixels(env, bitmap);

    // Guardar la imagen original en la carpeta
    createDirectoryIfNeeded(STORAGE_PATH);
    imwrite(STORAGE_PATH + "original.jpg", mat);

    return mat;
}

void calculateHuMoments(const Mat &image, vector<double> &huMoments) {
    createDirectoryIfNeeded(STORAGE_PATH);

    Mat gray, binary;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    imwrite(STORAGE_PATH + "1gray.jpg", gray);

    threshold(gray, binary, 235, 255, THRESH_BINARY_INV);
    imwrite(STORAGE_PATH + "2binary.jpg", binary);

    vector<vector<Point>> contours;
    findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        huMoments.assign(7, 0.0);
        return;
    }

    size_t largestContourIndex = 0;
    double maxArea = 0.0;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            largestContourIndex = i;
        }
    }

    // Dibujar el contorno más grande en una imagen negra
    Mat contourImage = Mat::zeros(binary.size(), CV_8UC3);
    drawContours(contourImage, contours, (int)largestContourIndex, Scalar(255, 255, 255), 1);
    imwrite(STORAGE_PATH + "3contour.jpg", contourImage);

    // Calcular Momentos de Hu
    Moments m = moments(contours[largestContourIndex]);
    double hu[7];
    HuMoments(m, hu);
    huMoments.assign(hu, hu + 7);
}

double euclideanDistance(const vector<double> &hu1, const vector<double> &hu2) {
    double sum = 0.0;
    for (size_t i = 0; i < hu1.size(); i++) {
        sum += pow(hu1[i] - hu2[i], 2);
    }
    return sqrt(sum);
}

vector<pair<string, vector<double>>> loadDataset(const string &filename) {
    ifstream file(filename);
    vector<pair<string, vector<double>>> dataset;
    string line;

    if (!file.is_open()) {
        LOGE("Error: No se pudo abrir el archivo %s", filename.c_str());
        return {};
    }

    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string className, filename;
        vector<double> huMoments;
        string value;

        getline(ss, className, ',');
        getline(ss, filename, ',');

        while (getline(ss, value, ',')) {
            huMoments.push_back(stod(value));
        }

        dataset.emplace_back(className, huMoments);
    }
    file.close();
    return dataset;
}

#include <sstream>
#include <cmath>

extern "C"
JNIEXPORT jstring JNICALL
Java_ec_edu_ups_huzernikeapp_MainActivity_classifyImage(JNIEnv *env, jobject, jobject bitmap, jstring datasetPath) {
    const char *nativeDatasetPath = env->GetStringUTFChars(datasetPath, 0);
    vector<pair<string, vector<double>>> dataset = loadDataset(nativeDatasetPath);
    env->ReleaseStringUTFChars(datasetPath, nativeDatasetPath);

    if (dataset.empty()) {
        return env->NewStringUTF("Error: No se pudo cargar el dataset.");
    }

    Mat mat = bitmapToMat(env, bitmap);
    vector<double> testHuMoments;
    calculateHuMoments(mat, testHuMoments);

    vector<pair<string, double>> distances;
    for (const auto &entry : dataset) {
        distances.emplace_back(entry.first, euclideanDistance(testHuMoments, entry.second));
    }

    sort(distances.begin(), distances.end(), [](const auto &a, const auto &b) {
        return a.second < b.second;
    });

    string predictedClass = distances.front().first;

    // **Normalización de los Momentos de Hu usando logaritmo**
    vector<double> normalizedHuMoments(7);
    for (int i = 0; i < 7; i++) {
        if (testHuMoments[i] != 0) {
            normalizedHuMoments[i] = -1.0 * copysign(log10(abs(testHuMoments[i])), testHuMoments[i]);
        } else {
            normalizedHuMoments[i] = 0; // Evitar log(0) que da -inf
        }
    }

    // **Formatear la salida con precisión**
    ostringstream result;
    result.precision(6);
    result << fixed; // Para mostrar los valores con precisión
    result << "Clasificación: " << predictedClass << "\nMomentos de Hu (log10):\n";

    for (int i = 0; i < 7; i++) {
        result << "Hu[" << (i + 1) << "]: " << normalizedHuMoments[i] << "\n";
    }

    return env->NewStringUTF(result.str().c_str());
}
