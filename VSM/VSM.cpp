// VSM.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <cstdio>
#include <cassert>
#include <math.h>
#include "HalfUtils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

using byte = unsigned char;

const int width = 1280;
const int height = 720;

byte* readIn(int width, int height, int byteSize, string filename);
void writeOut(int width, int height, int byteSize, __int8* data);
double* makeSquareDepth(float* depth);
float* byteToFloatPointer(byte* bytePointer);
__int8* floatToRGB(float* floatPointer);
__int8* floatToInt8(float* floatPointer);
double* floatToDouble(float* floatPointer);
double* Ex(double* dataDepth, half* hitMiss);
double Mean(double buffer[7][7], int hit,int w, int h);
double* Variance(double* ex, double* ex_2);
float maxValueF(float* data);
double maxValueD(double* data);
__int8* varianceRGB(__int8* variance, half* hitMiss);
float* convertVtoHue(double* variance, half* HitMiss);
__int8* convertHSLtoRGB(float* hue);
float* ExNew(float* dataDepth, half* hitMiss);
float Mean2(float buffer[3][3], float hit);

int main()
{
	int byteSizeDepth = 4;
	int byteSizeColor = 16;
	int byteSizeNormals = 8;
	float* floatDepth;
	float* floatColor;
	double* doubleDepth;
	half* halfNormals;

	string filenameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	string filenameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	string filenameNormals = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Normals.raw";

	byte* depth = readIn(width, height, byteSizeDepth, filenameDepth);
	byte* color = readIn(width, height, byteSizeColor, filenameColor);
	byte* normals = readIn(width, height, byteSizeNormals, filenameNormals);

	floatColor = (float*)color;
	floatDepth = (float*)depth;
	doubleDepth = floatToDouble(floatDepth);
	halfNormals = (half*)normals;

	printf("%i\n", *(halfNormals + 56020 * 8 + 7));
	printf("%6f\n", HalfToFloat(*(halfNormals + 56020 * 8 + 7)));

	__int8* intcolor = floatToRGB(floatColor);
	//writeOut(width, height, byteSizeDepth, intcolor);
	stbi_write_png("Output.png", width, height, 4, intcolor, 4 * width);
	//stbi_write_png("OutputDepth.png", width, height, 4, depth, 4 * width);
	double* squareDepth = makeSquareDepth(floatDepth);
	//__int8* intSquareDepth = floatToInt8(squareDepth);
	stbi_write_png("OutputDepthSquare.png", width, height, 4, squareDepth, 4 * width);
	printf(" first element of squareDepth: %6f\n", *(squareDepth));
	printf(" first element of squareDepth: %6f\n", *(floatDepth + 91557));
	printf(" first element of squareDepth: %6f\n", *(squareDepth + 91557));

	/*float test[7][7] = { {0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{ 0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5},
							{0.5, 0.4, 0.5, 0.1, 0.2, 0.4,0.5} };

	float mean = Mean(test, 49.000000);
	printf("%6f\n", mean);*/

	//meanValue = meanValue + (buffer[i][j] * (double)(1 / hitValue));
	double mean = 0.54887855 * (double)(1 / 49);
	printf("works fine %6f\n", 1.0/(double)25);

	double* ex = Ex(doubleDepth, halfNormals);
	double* ex_2 = Ex(squareDepth, halfNormals);
	double* variance = Variance(ex, ex_2);
	//__int8* intVariance = floatToInt8(variance);
	printf("maxValue Float %6f\n", maxValueF(floatDepth));
	printf("maxValue Double %6f\n", maxValueD(doubleDepth));
	printf("maxValue Doubleex %6f\n", maxValueD(ex));
	printf("maxValue Doubleex2 %6f\n", maxValueD(ex_2));
	printf("maxValue DoubleSquare %6f\n", maxValueD(squareDepth));
	printf("maxValue Variance %6f\n", maxValueD(variance));
	printf("maxValue %i\n", (__int8)maxValueD(variance));
	printf("%i\n", (__int8)DBL_MAX);
	//__int8* varianceColor = varianceRGB(intVariance, halfNormals);
	//writeOut(width, height, 1, intEx);
	//stbi_write_png("VarianceRGB.png", width, height, 3, varianceColor, 3 * width);
	float* hue = convertVtoHue(variance, halfNormals);
	__int8* varianceRGB = convertHSLtoRGB(hue);
	stbi_write_png("VarianceRGB.png", width, height, 3, varianceRGB, 3 * width);

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

void writeOut(int width, int height, int byteSize, __int8* data) {
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

float* byteToFloatPointer(byte* bytePointer) {//only for depth

	float buffer;
	byte bufferArray[4];
	float* floatPointer = new float[width*height];//only for depth, deshalb nicht *4

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

__int8* floatToRGB(float* floatPointer) {

	__int8* intPointer = new __int8[width*height * 4];

	for (int i = 0; i < width*height * 4; i++) {
		intPointer[i] = (__int8)((*(floatPointer + i)) * 255);
	}
	return intPointer;
}

__int8* floatToInt8(float* floatPointer) {

	__int8* intPointer = new __int8[width*height];

	for (int i = 0; i < width*height; i++) {
		/*if (((__int8)(*(floatPointer + i))) != 0) {
			printf("%i\n", ((__int8)(*(floatPointer + i))));
		} Test*/
		intPointer[i] = (__int8)(*(floatPointer + i));
	}
	return intPointer;
}

double* floatToDouble(float* floatPointer) {

	double* doublePointer = new double[width*height];

	for (int i = 0; i < width*height; i++) {
		doublePointer[i] = (double)(*(floatPointer + i));
	}
	return doublePointer;
}
double* Ex(double* dataDepth, half* hitMiss) {

	int hit = 0;
	double buffer[7][7];
	double* mean = new double[width*height];

	for (int h = 0; h < height; h++) {
		if (h == 0 || h == 1 || h == 2 || h == (height - 1) || h == (height - 2) || h == (height - 3)) {
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = 0.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {
				if (w == 0 || w == 1 || w == 2 || w == (width - 1) || w == (width - 2) || w == (width - 3)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = 0.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) == 1.0) {

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
						mean[(h*width) + w] = Mean(buffer, hit,w,h);
					}
					else {
						mean[(h*width) + w] = 0.0;
					}
					hit = 0;
				}
			}

		}

	}
	return mean;
}

float* ExNew(float* dataDepth, half* hitMiss) {

	int hit = 0;
	float buffer[3][3];
	float* mean = new float[width*height];

	for (int h = 0; h < height; h++) {
		if (h == 0 || h == (height - 1)) {
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = 0.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {
				if (w == 0 || w == (width - 1)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = 0.0;
					}
				}
				else {
					for (int j = 0; j < 3; j++) {

						for (int k = 0; k < 3; k++) {

							//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
							hit = hit + 1;
							buffer[j][k] = *(dataDepth + (k + (w - 1)) + ((j + (h - 1))*width) + 3);

						}
					}
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w) + 3))))) == 1.0) {
						mean[(h*width) + w] = Mean2(buffer, hit);
					}
					else {
						mean[(h*width) + w] = 0.0;
					}
					hit = 0;
				}
			}

		}

	}
	return mean;
}

double Mean(double buffer[7][7], int hitValue,int w,int h) {

	double meanValue = 0.0;

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			meanValue = meanValue + (buffer[i][j] * (1.0 / (double)hitValue));
		}
	}
	if (isinf(meanValue)) {
		int f = w;
		int t = h;
		meanValue = DBL_MAX;
		printf("%i\m", hitValue);
	}
	return meanValue;
}

float Mean2(float buffer[3][3], float hitValue) {

	float hit = (float)hitValue;
	float meanValue = 0.0;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			meanValue = meanValue + (buffer[i][j] * (1 / hit));
		}
	}
	return meanValue;
}

double* Variance(double* ex, double* ex_2) {

	double* variance = new double[width*height];


	for (int i = 0; i < (height*width); i++) {
		variance[i] = *(ex_2 + i) - ((*ex)*(*ex));
		if (isinf(variance[i])) {
			variance[i] = DBL_MAX;
		}
	}
	return variance;
}

float maxValueF(float* data) {

	float maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}

double maxValueD(double* data) {

	double maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}

__int8* varianceRGB(__int8* variance, half* hitMiss) {

	int j;
	__int8* varianceRGB = new __int8[width*height * 3];

	for (int i = 0; i < width*height; i++) {

		j = i * 3;

		if (((HalfToFloat(*(hitMiss + (4 * i + 3))))) != 1.0) {
			varianceRGB[j] = 0;
			varianceRGB[j + 1] = 0;
			varianceRGB[j + 2] = 0;
		}
		else {
			if (*(variance + i) < 20) {
				varianceRGB[j] = 0;
				varianceRGB[j + 1] = 180;
				varianceRGB[j + 2] = 0;
			}
			else if (*(variance + i) < 60) {
				varianceRGB[j] = 255;
				varianceRGB[j + 1] = 255;
				varianceRGB[j + 2] = 0;
			}
			else if (*(variance + i) < 255) {
				varianceRGB[j] = 255;
				varianceRGB[j + 1] = 0;
				varianceRGB[j + 2] = 0;
			}
		}
	}

	return varianceRGB;
}

float* convertVtoHue(double* variance, half* hitMiss) {

	double max = maxValueD(variance);
	float* hue = new float[width*height];


	for (int i = 0; i < width*height; i++) {
		hue[i] = fabs((((*(variance + i)) / max)*240.0) - 240.0);//convert variance to [0,240] and reverse value range

	}

	return hue;
}

__int8* convertHSLtoRGB(float* hue) {

	float c, x, m;
	__int8* rgb = new __int8[width*height * 3];
	int j;

	c = 1.f;
	m = 0;

	for (int i = 0; i < width*height; i++) {

		j = 3 * i;
		x = c * (1 - fabs(fmod((*(hue + i) / 60.f), 2) - 1));

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
			rgb[j] = 255;
			rgb[j + 1] = 255;
			rgb[j + 2] = 255;
		}
	}
	return rgb;
}





