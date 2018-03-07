// VSM.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <cstdio>
#include <cassert>
#include <math.h>
#include <vector>
#include "WindowsNumerics.h"
#include <string>
#include "HalfUtils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

using byte = unsigned char;

const int width = 1280;
const int height = 720;
const float FOVY = 0.785398163f;
half* normalsHitMiss = new half[width*height * 4];

byte* readIn(int width, int height, int byteSize, string filename);
void writeOut(int width, int height, int byteSize, __int8* data);
double* makeSquareDepth(float* depth);
__int8* floatToRGBA(float* floatPointer);
double* floatToDouble(float* floatPointer);
double* Ex(double* dataDepth, half* hitMiss, size_t size);
double Mean(double* buffer, int hit, size_t size);
double* Variance(double* ex, double* ex_2);
float maxValueF(float* data);
double maxValueD(double* data);
float* convertVtoHue(double* variance);
__int8* convertHSLtoRGB(float* hue);
double* Chebyshev(double* variance, double* ex, double* depth);
float* newDepth(double* variance, double* depth, double* ex);
__int8* Normals(float* depth, half* hitMiss);
std::vector<float> Pos3D(std::vector<int> position, std::vector<int> offset, float depth);
__int8* Normals3D(float* depth, half* hitMiss);
void smooth(double* oldDepth, double* oldVariance, double* oldMean, half* hitMiss, int iterations, size_t size);
double maxDepthD(double* depth, half* hitMiss);
double weightedAverage(double buffer[3][3], int hitValue, double maxValue);

int main()
{
	int byteSizeDepth = 4;		//
	int byteSizeColor = 16;		//different byteSizes for different .raw files
	int byteSizeNormals = 8;	//
	float* floatDepth;
	float* floatColor;
	double* doubleDepth;
	double* squaredDepth;
	half* halfNormals;

	//string filenameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	//string filenameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	//string filenameNormals = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Normals.raw";

	string filenameDepth = "Dragon_Spheres_1.5/framedump_09012018_115927_Depth.raw";		//
	string filenameColor = "Dragon_Spheres_1.5/framedump_09012018_115927_Colors.raw";		//input files
	string filenameNormals = "Dragon_Spheres_1.5/framedump_09012018_115927_Normals.raw";	//

	//string filenameDepth = "Dragon_Spheres_1.0/framedump_09012018_115957_Depth.raw";
	//string filenameColor = "Dragon_Spheres_1.0/framedump_09012018_115957_Colors.raw";
	//string filenameNormals = "Dragon_Spheres_1.0/framedump_09012018_115957_Normals.raw";

	//string filenameDepth = "Audimax_Spheres_1.5/framedump_09012018_141355_Depth.raw";
	//string filenameColor = "Audimax_Spheres_1.5/framedump_09012018_141355_Colors.raw";
	//string filenameNormals = "Audimax_Spheres_1.5/framedump_09012018_141355_Normals.raw";

	byte* depth = readIn(width, height, byteSizeDepth, filenameDepth);			//
	byte* color = readIn(width, height, byteSizeColor, filenameColor);			//readIn .raw files and store them
	byte* normals = readIn(width, height, byteSizeNormals, filenameNormals);	//

	floatColor = (float*)color;						//
	floatDepth = (float*)depth;						//
	doubleDepth = floatToDouble(floatDepth);		//change units of input
	squaredDepth = makeSquareDepth(floatDepth);		//
	halfNormals = (half*)normals;					//
	normalsHitMiss = (half*)normals;				//



	__int8* intcolor = floatToRGBA(floatColor);
	stbi_write_png("Output.png", width, height, 4, intcolor, 4 * width);


	double* mean = Ex(doubleDepth, halfNormals, 3);;
	double* meanSquared = Ex(squaredDepth, halfNormals, 3);
	double* variance = Variance(mean, meanSquared);

	double maxV = maxValueD(variance);

	__int8* normals3D = Normals3D(floatDepth, halfNormals);
	stbi_write_png("Normals3D.png", width, height, 3, normals3D, 3 * width);

	__int8* normalsRGB = Normals(floatDepth, halfNormals);
	stbi_write_png("Normals.png", width, height, 3, normalsRGB, 3 * width);

	
	float* hue = convertVtoHue(variance);
	__int8* varianceRGB = convertHSLtoRGB(hue);
	stbi_write_png("VarianceRGB.png", width, height, 3, varianceRGB, 3 * width);


	smooth(doubleDepth, variance, mean, halfNormals, 5, 3);
}



byte* readIn(int width, int height, int byteSize, string filename)//liest Datei ein und speichert sie bei filename
{

	byte* rawData = new byte[width*height*byteSize];
	{
		FILE* file = nullptr;
		fopen_s(&file, filename.c_str(), "rb");
		fread(rawData, width * height * byteSize, 1, file);
		fclose(file);
	}
	return rawData;
}

void writeOut(int width, int height, int byteSize, __int8* data) {//schreibt ggf. data in myfile.raw
	FILE * pfile;
	pfile = fopen("myfile.raw", "wb");
	fwrite(data, width*height*byteSize, 1, pfile);
	fclose(pfile);
}

double* makeSquareDepth(float* depth)//quadriert Zelleninhalte des Arrays bei depth
{

	double* squareDepth = new double[width*height];
	for (int i = 0; i < (width*height); i++) {
		squareDepth[i] = (*depth)*(*depth);
		if (isinf(squareDepth[i])) {
			squareDepth[i] = DBL_MAX;
		}
		depth++;
	}
	return squareDepth;
}

__int8* floatToRGBA(float* floatPointer) {//macht aus float-Values Werte zwischen 0 und 255 (RGBA)

	__int8* intPointer = new __int8[width*height * 4];

	for (int i = 0; i < width*height * 4; i++) {
		intPointer[i] = (__int8)((*(floatPointer + i)) * 255);
	}
	return intPointer;
}


double* floatToDouble(float* floatPointer) {//macht aus einem float Pointer einen Double Pointer

	double* doublePointer = new double[width*height];

	for (int i = 0; i < width*height; i++) {
		doublePointer[i] = (double)(*(floatPointer + i));
	}
	return doublePointer;
}


double* Ex(double* dataDepth, half* hitMiss, size_t size) {//berechnet den Erwartungswert von dataDepth mit einem 3*3 Filter


	double* buffer = new double[size*size];
	int hit = 0;
	double* mean = new double[width*height];
	double maxDepth = maxDepthD(dataDepth, hitMiss);

	for (int h = 0; h < height; h++) {
		if (h <= size/2 || h >= (height - size/2)) {//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
				if (w <= size/2 || w >= (width - size/2)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = -1.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {//Wenn kein Miss, dann ...

						for (int j = 0; j < size; j++) {

							for (int k = 0; k < size; k++) {

								if (HalfToFloat(hitMiss[((4 * ((k + (w - size/2)) + ((j + (h - size/2))*width))) + 3)]) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
									hit = hit + 1;
									buffer[j * size + k] = dataDepth[(k + (w - size/2)) + ((j + (h - size/2))*width)];
								}
								else {
									buffer[j*size+k] = 0.0;
								}
							}
						}
						if (hit > 1) {//Füllt die Löcher im Objekt mit Erwartungswerten
							mean[(h*width) + w] = Mean(buffer, hit, size);
						}
						else {
							mean[(h*width) + w] = -1.0;
						}
					}
					else {
						mean[(h*width) + w] = -1.0;
					}
				}
				hit = 0;
			}
		}
	}
	delete buffer;
	return mean;
	delete mean;
}

double Mean(double* buffer, int hitValue, size_t size) {//Berechnet den Erwartungswert von 3*3 Werten

	double meanValue = 0.0;

	for (int i = 0; i < size*size; i++) {
			meanValue += (buffer[i] * (1.0 / (double)hitValue));
	}
	return meanValue;
}


double* Variance(double* ex, double* ex_2) {//Berechnet die Varianz aus Erwartungswert(x) und Erwartungswert(x^2)

	double* variance = new double[width*height];


	for (int i = 0; i < (height*width); i++) {
		variance[i] = *(ex_2 + i) - ((*(ex + i))*(*(ex + i)));
		if (isinf(variance[i])) {
			variance[i] = DBL_MAX;
		}
	}
	return variance;
}

float maxValueF(float* data) {//Gibt den maximalen Float Wert des data pointers zurück

	float maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}

double maxValueD(double* data) {//Gibt den maximalen double Wert des data pointers zurück

	double maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}


float* convertVtoHue(double* variance) {//Konvertiert die Varianzwerte in HUE-Werte

	double max = maxValueD(variance);
	float* hue = new float[width*height];


	for (int i = 0; i < width*height; i++) {
		hue[i] = fabs((((*(variance + i)) / max)*240.0) - 240.0);//convert variance to [0,240] and reverse value range
	}

	return hue;
}

__int8* convertHSLtoRGB(float* hue) {//Konvertiert die von convertVtoHue berechneten HUE Werte mit festen Saturation und Lightness Werten zu RGB-Werte für stbi_write_png

	float c, x, m;
	__int8* rgb = new __int8[width*height * 3];
	int j;

	c = 1.f;
	m = 0;

	for (int i = 0; i < width*height; i++) {

		j = 3 * i;
		x = c * (1.f - fabs(fmod((*(hue + i) / 60.f), 2.f) - 1.f));

		if (*(hue + i) < 60) {
			rgb[j] = (__int8)(c * 255);
			rgb[j + 1] = (__int8)(x * 255);
			rgb[j + 2] = 0;
		}
		else if (*(hue + i) < 120) {
			rgb[j] = (__int8)(x * 255);
			rgb[j + 1] = (__int8)(c * 255);
			rgb[j + 2] = 0;
		}
		else if (*(hue + i) < 180) {
			rgb[j] = 0;
			rgb[j + 1] = (__int8)(c * 255);
			rgb[j + 2] = (__int8)(x * 255);
		}
		else if (*(hue + i) < 240) {
			rgb[j] = 0;
			rgb[j + 1] = (__int8)(x * 255);
			rgb[j + 2] = (__int8)(c * 255);
		}
		else if (*(hue + i) < 300) {
			rgb[j] = (__int8)(x * 255);
			rgb[j + 1] = 0;
			rgb[j + 2] = (__int8)(c * 255);
		}
		else if (*(hue + i) < 360) {
			rgb[j] = (__int8)(c * 255);
			rgb[j + 1] = 0;
			rgb[j + 2] = (__int8)(x * 255);
		}
		else {
			rgb[j] = 0;
			rgb[j + 1] = 0;
			rgb[j + 2] = 0;
		}
	}
	return rgb;
}

double* Chebyshev(double* variance, double* ex, double* depth) {//Berechnet mit der Chebyshev Ungleichung die prozentuale Belichtung

	double* cheby = new double[width*height];

	for (int i = 0; i < width*height; i++) {
		if (*(depth + i) > (*(ex + i)) && !(isinf(*(depth + i))) && (*(ex + i)) >= 0) {
			cheby[i] = ((*(variance + i)) * (*(variance + i))) / (((*(variance + i)) * (*(variance + i))) + (*(depth + i) - (*(ex + i))));
		}
		else {
			cheby[i] = -1.0;
		}
	}
	return cheby;
}

float* newDepth(double* variance, double* depth, double* ex) {//Berechnet den neuen Tiefenwert um Löcher zu schließen und zu glätten

	float* newDepth = new float[width*height];
	double max = maxValueD(variance);

	for (int i = 0; i < width*height; i++) {
		if (((*(variance + i)) > (0)) && (*(variance + i)) != -2) {
			newDepth[i] = *(ex + i);
			::normalsHitMiss[4 * i + 3] = FloatToHalf(1.0);
		}
		else {
			newDepth[i] = *(depth + i);
		}
	}
	return newDepth;
}

__int8* Normals(float* depth, half* hitMiss) {//Berechnet die Normalen

	int j;
	float length;
	float* normals = new float[width*height * 3];
	__int8* normalsRGB = new __int8[width*height * 3];

	for (int i = 0; i < width*(height - 1); i++) {
		j = i * 3;

		if (HalfToFloat(*(hitMiss + 4 * i + 3)) == 1.0) {
			normals[j] = ((*(depth + i + 1)) - (*(depth + i)));
			normals[j + 1] = ((*(depth + i + 1280)) - (*(depth + i)));
			normals[j + 2] = 1;
			length = sqrt((normals[j] * normals[j]) + (normals[j + 1] * normals[j + 1]) + (normals[j + 2] * normals[j + 2]));
			normals[j] = (normals[j] / length)*0.5 + 0.5;
			normals[j + 1] = (normals[j + 1] / length)*0.5 + 0.5;
			normals[j + 2] = (normals[j + 2] / length)*0.5 + 0.5;

			normalsRGB[j] = normals[j] * 255;
			normalsRGB[j + 1] = normals[j + 1] * 255;
			normalsRGB[j + 2] = normals[j + 2] * 255;
		}
		else {
			normalsRGB[j] = 0;
			normalsRGB[j + 1] = 0;
			normalsRGB[j + 2] = 0;
		}
	}

	return normalsRGB;
}

std::vector<float> Pos3D(std::vector<int> position, std::vector<int> offset, float depth) {

	float scaleY = tan(FOVY / 2.0f);
	float scaleX = scaleY * width / height;

	std::vector<float> pos (3);

	pos[0] = ((float(position[0] + offset[0]) / float(width))*2.0f - 1.0f)*scaleX*depth;
	pos[1] = ((float(position[1] + offset[1]) / float(height))*2.0f - 1.0f)*(-scaleY)*depth;
	pos[2] = depth;

	return pos;
}

__int8* Normals3D(float* depth, half* hitMiss) {

	int k;
	std::vector<int> pos (2), offset (2);
	std::vector<float> a (3), b (3), c (3), d (3), e (3);
	float lengthc, lengthd, lengthe;
	__int8* normals3DRGB = new __int8[3 * width*height];

	for (int i = 1; i < height - 1; i++) {
		for (int j = 1; j < width - 1; j++) {

			k = j * 3;

			if (HalfToFloat(hitMiss[4 * (i*1280+j) + 3]) == 1.0){
			

			pos[0] = i;
			pos[1] = j;

			offset[0] = 1;
			offset[1] = 0;
			a = Pos3D(pos, offset, depth[i * 1280 + j+1]);
			offset[0] = -1;
			offset[1] = 0;
			b = Pos3D(pos, offset, depth[i * 1280 + j-1]);
			c[0] = a[0] - b[0];
			c[1] = a[1] - b[1];
			c[2] = a[2] - b[2];

			offset[0] = 0;
			offset[1] = 1;
			a = Pos3D(pos, offset, depth[(i+1) * 1280 + j]);
			offset[0] = 0;
			offset[1] = -1;
			b = Pos3D(pos, offset, depth[(i-1) * 1280 + j]);
			d[0] = a[0] - b[0];
			d[1] = a[1] - b[1];
			d[2] = a[2] - b[2];

			lengthc = sqrt((c[0] * c[0] + c[1] * c[1] + c[2] * c[2]));
			c[0] = c[0] / lengthc;
			c[1] = c[1] / lengthc;
			c[2] = c[2] / lengthc;
			lengthd = sqrt((d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
			d[0] = d[0] / lengthd;
			d[1] = d[1] / lengthd;
			d[2] = d[2] / lengthd;

			e[0] = c[1] * d[2] - c[2] * d[1];
			e[1] = c[2] * d[0] - c[0] * d[2];
			e[2] = c[0] * d[1] - c[1] * b[0];
			lengthe = sqrt((e[0] * e[0] + e[1] * e[1] + e[2] * e[2]));

			normals3DRGB[i * 3 * 1280 + k] = ((e[0] / lengthe)*0.5 + 0.5)*255;
			normals3DRGB[i * 3 * 1280 + k + 1] = ((e[1] / lengthe)*0.5 + 0.5)*255;
			normals3DRGB[i * 3 * 1280 + k + 2] = ((e[2] / lengthe)*0.5 + 0.5)*255;
			}
			else {
				normals3DRGB[i * 3 * width + k] = 0;
				normals3DRGB[i * 3 * width + k+1] = 0;
				normals3DRGB[i * 3 * width + k+2] = 0;
			}
		}
	}
	return normals3DRGB;
}

void smooth(double* oldDepth, double* oldVariance, double* oldMean, half* hitMiss, int iterations, size_t size) {//Glättet das Bild


	for (int i = 0; i < iterations; i++) {

		string s = "NewWeightedVariance" + std::to_string(i) + ".png";
		const char* c = s.c_str();

		string r = "NewWeightedNormals3D" + std::to_string(i) + ".png";
		const char* d = r.c_str();

		float* newFloatDepth = newDepth(oldVariance, oldDepth, oldMean);
		double* newSquaredDepth = makeSquareDepth(newFloatDepth);
		double* newDoubleDepth = floatToDouble(newFloatDepth);
		double* newMean = Ex(newDoubleDepth, hitMiss, size);
		double* newSquaredMean = Ex(newSquaredDepth, hitMiss, size);
		double* newVariance = Variance(newMean, newSquaredMean);
		float* hue = convertVtoHue(newVariance);

		double max = maxValueD(newVariance);

		__int8* newDepthRGB = convertHSLtoRGB(hue);
		stbi_write_png(c, width, height, 3, newDepthRGB, 3 * width);


		__int8* newNormalsRGB = Normals3D(newFloatDepth, normalsHitMiss);
		stbi_write_png(d, width, height, 3, newNormalsRGB, 3 * width);

		oldDepth = newDoubleDepth;
		oldVariance = newVariance;
		oldMean = newMean;
		hitMiss = normalsHitMiss;
	}



}

double maxDepthD(double* depth, half* hitMiss) {

	double maxDepth = 0;

	for (int i = 0; i < width*height; i++) {
		if (depth[i] > maxDepth && HalfToFloat(*(hitMiss + 4 * i + 3)) == 1.0) {
			maxDepth = depth[i];
		}
	}
	return maxDepth;
}

double weightedAverage(double buffer[3][3], int hitValue, double maxDepth) {

	double meanValue = 0.0;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			meanValue = meanValue + (buffer[i][j] * (1.0 / (double)hitValue));
		}
	}
	return meanValue*(1-buffer[1][1]/maxDepth);

}