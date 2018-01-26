// VSM.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <cstdio>
#include <cassert>
#include "HalfUtils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

using byte = unsigned char;

const int width = 1280;
const int height = 720;

byte* readIn(int width, int height, int byteSize, string filename);
void writeOut(int width, int height, int byteSize, __int8* data);
float* makeSquareDepth(float* depth);
float* byteToFloatPointer(byte* bytePointer);
__int8* floatToRGB(float* floatPointer);
__int8* floatToInt8(float* floatPointer);
float* Ex(float* dataDepth, half* hitMiss);
float Mean(float buffer[7][7], float hit);
float* Variance(float* ex, float* ex_2);
float maxValue(float* data);

int main()
{
	int byteSizeDepth = 4;
	int byteSizeColor = 16;
	int byteSizeNormals = 8;
	float* floatDepth;
	float* floatColor;
	half* halfNormals;

	string filenameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	string filenameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	string filenameNormals = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Normals.raw";

	byte* depth = readIn(width, height, byteSizeDepth, filenameDepth);
	byte* color = readIn(width, height, byteSizeColor, filenameColor);
	byte* normals = readIn(width, height, byteSizeNormals, filenameNormals);

	floatColor = (float*)color;
	floatDepth = (float*)depth;
	halfNormals = (half*)normals;

	printf("%i\n", *(halfNormals + 56020 * 8 + 7));
	printf("%6f\n", HalfToFloat(*(halfNormals + 56020 * 8 + 7)));

	__int8* intcolor = floatToRGB(floatColor);
	//writeOut(width, height, byteSizeDepth, intcolor);
	stbi_write_png("Output.png", width, height, 4, intcolor, 4 * width);
	//stbi_write_png("OutputDepth.png", width, height, 4, depth, 4 * width);
	float* squareDepth = makeSquareDepth(floatDepth);
	__int8* intSquareDepth = floatToInt8(squareDepth);
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
	
	float* ex = Ex(floatDepth, halfNormals);
	float* ex_2 = Ex(squareDepth, halfNormals);
	float* variance = Variance(ex, ex_2);
	__int8* intVariance = floatToInt8(variance);
	printf("the first element of ex: %6f\n", *(ex));
	__int8* intEx = floatToInt8(ex);
	printf("the first element of ex: %i\n", *(intEx));
	printf("maxValue %i\n", (__int8)maxValue(variance));
	writeOut(width, height, 1, intEx);
	stbi_write_png("Variance.png", width, height, 4, variance, 4*width);

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

float* makeSquareDepth(float* depth)//quadriert Zelleninhalte des Arrays bei depth
{

	float* squareDepth = new float[width*height];
	for (int i = 0; i < (width*height); i++) {
		squareDepth[i] = (*depth)*(*depth);
		if (isinf(squareDepth[i])) {
			squareDepth[i] = 0.0;
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

float* Ex(float* dataDepth, half* hitMiss) {

	float hit = 0.000000;
	float buffer[7][7];
	float* mean = new float[width*height];

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
					for (int j = 0; j < 7; j++) {

						for (int k = 0; k < 7; k++) {

							if (HalfToFloat(*(hitMiss + (4 * (k + (w - 3))) + ((j + (h - 3))*width) + 3)) != 0.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
								hit = hit + 1.000000;
								buffer[j][k] = *(dataDepth + (k + (w - 3)) + ((j + (h - 3))*width) + 3);
							}
							else {
								buffer[j][k] = 0.0;
							}
						}
					}
					if (hit != 0 && ((HalfToFloat(*(hitMiss + (4 * ((h*width) + w) +3))))) == 1.0) {
						mean[(h*width) + w] = Mean(buffer, hit);
					}
					else {
						mean[(h*width) + w] = 0.0;
					}
					hit = 0.000000;
				}
			}

		}

	}
	return mean;
}

float Mean(float buffer[7][7], float hit) {

	float meanValue = 0.000000;

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			meanValue = meanValue + (buffer[i][j] * (1 / hit));
		}
	}
	return meanValue;
}

float* Variance(float* ex, float* ex_2) {

	float* variance = new float[width*height];


	for (int i = 0; i < (height*width); i++) {
		variance[i] = *(ex_2 + i) - ((*ex)*(*ex));
	}
	return variance;
}

float maxValue(float* data) {

	float maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}