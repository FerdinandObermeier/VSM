// VSM.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <cstdio>
#include <cassert>
#include <math.h>
#include <string>
#include "HalfUtils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

using byte = unsigned char;

const int width = 1280;
const int height = 720;
half* normalsHitMiss = new half[width*height * 4];

byte* readIn(int width, int height, int byteSize, string filename);
void writeOut(int width, int height, int byteSize, __int8* data);
double* makeSquareDepth(float* depth);
float* byteToFloatPointer(byte* bytePointer);
__int8* floatToRGBA(float* floatPointer);
__int8* floatToInt8(float* floatPointer);
double* floatToDouble(float* floatPointer);
double* Ex(double* dataDepth, half* hitMiss);
double* Ex2(double* dataDepth, half* hitMiss);
double* Ex3(double* dataDepth, half* hitMiss);
double Mean(double buffer[7][7], int hit);
double Mean2(double buffer[3][3], int hit);
double Mean3(double buffer[15][15], int hitValue);
double* Variance(double* ex, double* ex_2);
float maxValueF(float* data);
double maxValueD(double* data);
float* convertVtoHue(double* variance);
__int8* convertHSLtoRGB(float* hue);
double* Chebyshev(double* variance, double* ex, double* depth);
float* newDepth(double* variance, double* depth, double* ex);
__int8* Normals(float* depth, half* hitMiss);
void smooth(double* oldDepth, double* oldVariance, double* oldMean, half* hitMiss, int iterations);

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

	//printf("%i\n", *(halfNormals + 56020 * 8 + 7));
	//printf("%6f\n", HalfToFloat(*(halfNormals + 56020 * 8 + 7)));


	__int8* intcolor = floatToRGBA(floatColor);
	//writeOut(width, height, byteSizeDepth, intcolor);
	stbi_write_png("Output.png", width, height, 4, intcolor, 4 * width);
	//stbi_write_png("OutputDepth.png", width, height, 4, depth, 4 * width);
	//__int8* intSquareDepth = floatToInt8(squareDepth);
	//stbi_write_png("OutputDepthSquare.png", width, height, 4, squareDepth, 4 * width);
	//printf(" first element of squareDepth: %6f\n", *(squareDepth));
	//printf(" first element of squareDepth: %6f\n", *(floatDepth + 91557));
	//printf(" first element of squareDepth: %6f\n", *(squareDepth + 91557));


	/*double* ex = Ex(doubleDepth, halfNormals);
	double* ex_2 = Ex(squareDepth, halfNormals);
	double* variance = Variance(ex, ex_2);

	double* ex2 = Ex2(doubleDepth, halfNormals);
	double* ex2_2 = Ex2(squareDepth, halfNormals);
	double* variance2 = Variance(ex2, ex2_2);*/

	double* mean = Ex3(doubleDepth, halfNormals);;			
	double* meanSquared = Ex3(squaredDepth, halfNormals);
	double* variance = Variance(mean, meanSquared);
	
	__int8* normalsRGB = Normals(floatDepth, halfNormals);
	stbi_write_png("Normals.png", width, height, 3, normalsRGB, 3 * width);

/*
	//__int8* intVariance = floatToInt8(variance);
	printf("maxValue Float %6f\n", maxValueF(floatDepth));
	printf("maxValue Double %6f\n", maxValueD(doubleDepth));
	printf("maxValue Doubleex %6f\n", maxValueD(ex));
	printf("maxValue Doubleex2 %6f\n", maxValueD(ex_2));
	printf("maxValue DoubleSquare %6f\n", maxValueD(squareDepth));
	printf("maxValue Variance %6f\n", maxValueD(variance));
	printf("maxValue %i\n", (__int8)maxValueD(variance));
	printf("%i\n", (__int8)DBL_MAX);
	//writeOut(width, height, 1, intEx);

	float* hue = convertVtoHue(variance);
	__int8* varianceRGB = convertHSLtoRGB(hue);
	stbi_write_png("VarianceRGB.png", width, height, 3, varianceRGB, 3 * width);

	float* hue2 = convertVtoHue(variance2);
	__int8* varianceRGB2 = convertHSLtoRGB(hue2);
	stbi_write_png("VarianceRGB2.png", width, height, 3, varianceRGB2, 3 * width);

	float* hue3 = convertVtoHue(variance3);
	__int8* varianceRGB3 = convertHSLtoRGB(hue3);
	stbi_write_png("VarianceRGB3.png", width, height, 3, varianceRGB3, 3 * width);

	double* cheby = Chebyshev(variance3, ex3, doubleDepth);
	float* hue4 = convertVtoHue(cheby);
	__int8* chebyRGB = convertHSLtoRGB(hue4);
	stbi_write_png("Chebyshev.png", width, height, 3, chebyRGB, 3 * width);

	printf("%f\n", maxValueD(cheby));

	float* newFloatDepth = newDepth(variance3, doubleDepth, ex3);
	double* newSquareDepth = makeSquareDepth(newFloatDepth);
	double* newDoubleDepth = floatToDouble(newFloatDepth);
	double* newEx = Ex3(newDoubleDepth, halfNormals);
	double* newEx2 = Ex3(newSquareDepth, halfNormals);
	double* newVariance = Variance(newEx, newEx2);
	float* hue5 = convertVtoHue(newVariance);
	__int8* newDepthRGB = convertHSLtoRGB(hue5);
	stbi_write_png("Fill.png", width, height, 3, newDepthRGB, 3 * width);

	

	__int8* newNormalsRGB = Normals(newFloatDepth, normalsHitMiss);
	stbi_write_png("NewNormals.png", width, height, 3, newNormalsRGB, 3 * width);


	float* new2FloatDepth = newDepth(newVariance, newDoubleDepth, newEx);
	double* new2SquareDepth = makeSquareDepth(new2FloatDepth);
	double* new2DoubleDepth = floatToDouble(new2FloatDepth);
	double* new2Ex = Ex3(new2DoubleDepth, normalsHitMiss);
	double* new2Ex2 = Ex3(new2SquareDepth, normalsHitMiss);
	double* new2Variance = Variance(new2Ex, new2Ex2);
	float* hue6 = convertVtoHue(new2Variance);
	__int8* new2DepthRGB = convertHSLtoRGB(hue6);
	stbi_write_png("FillNew.png", width, height, 3, new2DepthRGB, 3 * width);

	__int8* new2NormalsRGB = Normals(new2FloatDepth, normalsHitMiss);
	stbi_write_png("New2Normals.png", width, height, 3, new2NormalsRGB, 3 * width);*/


	smooth(doubleDepth, variance, mean, halfNormals, 5);
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

float* byteToFloatPointer(byte* bytePointer) {//konvertiert den depth byte Pointer zu einem float Pointer

	float buffer;
	byte bufferArray[4];
	float* floatPointer = new float[width*height];//nur für depth, deshalb nicht *4

	for (int i = 0; i < width*height; i = i + 4) {
		bufferArray[0] = *(bytePointer + i);
		bufferArray[1] = *(bytePointer + i + 1);
		bufferArray[2] = *(bytePointer + i + 2);
		bufferArray[3] = *(bytePointer + i + 3);
		memcpy(&buffer, &bufferArray, sizeof(buffer));
		floatPointer[i / 4] = buffer;
		buffer = -107374176.;
	}
	return floatPointer;
}

__int8* floatToRGBA(float* floatPointer) {//macht aus float-Values Werte zwischen 0 und 255 (RGBA)

	__int8* intPointer = new __int8[width*height * 4];

	for (int i = 0; i < width*height * 4; i++) {
		intPointer[i] = (__int8)((*(floatPointer + i)) * 255);
	}
	return intPointer;
}

__int8* floatToInt8(float* floatPointer) {//macht aus einem float Pointer einen int8 Pointer

	__int8* intPointer = new __int8[width*height];

	for (int i = 0; i < width*height; i++) {
		/*if (((__int8)(*(floatPointer + i))) != 0) {
			printf("%i\n", ((__int8)(*(floatPointer + i))));
		} Test*/
		intPointer[i] = (__int8)(*(floatPointer + i));
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

double* Ex(double* dataDepth, half* hitMiss) {//berechnet den Erwartungswert von dataDepth mit einem 7*7 Filter

	int hit = 0;
	double buffer[7][7];
	double* mean = new double[width*height];

	for (int h = 0; h < height; h++) {
		if (h == 0 || h == 1 || h == 2 || h == (height - 1) || h == (height - 2) || h == (height - 3)) {//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {
				if (w == 0 || w == 1 || w == 2 || w == (width - 1) || w == (width - 2) || w == (width - 3)) {//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
					for (int n = 3; n < height - 3; n++) {
						mean[w + (n * width)] = -1.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {//Wenn kein Miss, dann ...

						for (int j = 0; j < 7; j++) {

							for (int k = 0; k < 7; k++) {

								if (HalfToFloat(*(hitMiss + ((4 * ((k + (w - 3)) + ((j + (h - 3))*width))) + 3))) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
									hit = hit + 1;
									buffer[j][k] = *(dataDepth + (k + (w - 3)) + ((j + (h - 3))*width));
								}
								else {
									buffer[j][k] = 0.0;
								}
							}
						}
						if (hit >24) {//Füllt die Löcher im Objekt mit Erwartungswerten
							mean[(h*width) + w] = Mean(buffer, hit);
							/**/
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
	return mean;
}

double* Ex2(double* dataDepth, half* hitMiss) {//berechnet den Erwartungswert von dataDepth mit einem 3*3 Filter

	int hit = 0;
	double buffer[3][3];
	double* mean = new double[width*height];

	for (int h = 0; h < height; h++) {
		if (h == 0 || h == (height - 1)) {//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
				if (w == 0 || w == (width - 1)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = -1.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {//Wenn kein Miss, dann ...

						for (int j = 0; j < 3; j++) {

							for (int k = 0; k < 3; k++) {

								if (HalfToFloat(*(hitMiss + ((4 * ((k + (w - 1)) + ((j + (h - 1))*width))) + 3))) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
									hit = hit + 1;
									buffer[j][k] = *(dataDepth + (k + (w - 1)) + ((j + (h - 1))*width));
								}
								else {
									buffer[j][k] = 0.0;
								}
							}
						}
						if (hit >1) {//Füllt die Löcher im Objekt mit Erwartungswerten
							mean[(h*width) + w] = Mean2(buffer, hit);
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
	return mean;
}

double* Ex3(double* dataDepth, half* hitMiss) {//berechnet den Erwartungswert von dataDepth mit einem 15*15 Filter

	int hit = 0;
	double buffer[15][15];
	double* mean = new double[width*height];

	for (int h = 0; h < height; h++) {
		if (h == 0 || h == 1 || h == 2 || h == 3 || h == 4 || h == 5 || h == 6 || h == 7 || h == (height - 1) || h == (height - 2) || h == (height - 3) || h == (height - 4) || h == (height - 5) || h == (height - 6) || h == (height - 7)) {//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {
				if (w == 0 || w == 1 || w == 2 || w == 3 || w == 4 || w == 5 || w == 6 || w == 7 || w == (width - 1) || w == (width - 2) || w == (width - 3) || w == (width - 4) || w == (width - 5) || w == (width - 6) || w == (width - 7)) {//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt, da der Filter hierfür zu groß ist
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = -1.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {//Wenn kein Miss, dann ...

						for (int j = 0; j < 15; j++) {

							for (int k = 0; k < 15; k++) {

								if (HalfToFloat(*(hitMiss + ((4 * ((k + (w - 7)) + ((j + (h - 7))*width))) + 3))) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
									hit = hit + 1;
									buffer[j][k] = *(dataDepth + (k + (w - 7)) + ((j + (h - 7))*width));
								}
								else {
									buffer[j][k] = 0.0;
								}
							}
						}
						if (hit > 112) {//Füllt die Löcher im Objekt mit Erwartungswerten
							mean[(h*width) + w] = Mean3(buffer, hit);
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
	return mean;
}

double Mean(double buffer[7][7], int hitValue) {//Berechnet den Erwartungswert von 7*7 Werten

	double meanValue = 0.0;

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			meanValue = meanValue + (buffer[i][j] * (1.0 / (double)hitValue));
		}
	}
	return meanValue;
}

double Mean2(double buffer[3][3], int hitValue) {//Berechnet den Erwartungswert von 3*3 Werten

	double meanValue = 0.0;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			meanValue = meanValue + (buffer[i][j] * (1.0 / (double)hitValue));
		}
	}
	return meanValue;
}

double Mean3(double buffer[15][15], int hitValue) {//Berechnet den Erwartungswert von 15*15 Werten

	double meanValue = 0.0;

	for (int i = 0; i < 15; i++) {
		for (int j = 0; j < 15; j++) {
			meanValue = meanValue + (buffer[i][j] * (1.0 / (double)hitValue));
		}
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

	for (int i = 0; i < width*height; i++) {
		if ((*(variance + i)) != 0 && (*(variance+i))!=-2) {
			newDepth[i] = *(ex + i);
			::normalsHitMiss[4*i+3] = FloatToHalf(1.0);
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
		}
		else {
			normals[j] = 0;
			normals[j + 1] = 0;
			normals[j + 2] = 0;
		}
	}

	float max = maxValueF(normals);


	for (int k = 0; k < width*(height - 1); k++) {
		j = k * 3;

		normalsRGB[j] = normals[j] * 255;
		normalsRGB[j + 1] = normals[j + 1] * 255;
		normalsRGB[j + 2] = normals[j + 2] * 255;

	}

	return normalsRGB;
}

void smooth(double* oldDepth, double* oldVariance, double* oldMean, half* hitMiss, int iterations) {//Glättet das Bild

	
	for (int i = 0; i < iterations; i++) {

		string s = "Fill"+std::to_string(i)+".png";
		const char* c = s.c_str();

		string r = "NewNormals" + std::to_string(i) + ".png";
		const char* d = r.c_str();

		float* newFloatDepth = newDepth(oldVariance, oldDepth, oldMean);
		double* newSquaredDepth = makeSquareDepth(newFloatDepth);
		double* newDoubleDepth = floatToDouble(newFloatDepth);
		double* newMean = Ex3(newDoubleDepth, hitMiss);
		double* newSquaredMean = Ex3(newSquaredDepth, hitMiss);
		double* newVariance = Variance(newMean, newSquaredMean);
		float* hue = convertVtoHue(newVariance);


		__int8* newDepthRGB = convertHSLtoRGB(hue);
		stbi_write_png(c, width, height, 3, newDepthRGB, 3 * width);

	
		__int8* newNormalsRGB = Normals(newFloatDepth, normalsHitMiss);
		stbi_write_png(d, width, height, 3, newNormalsRGB, 3 * width);

		oldDepth = newDoubleDepth;
		oldVariance = newVariance;
		oldMean = newMean;
		hitMiss = normalsHitMiss;
	}
	
	

}