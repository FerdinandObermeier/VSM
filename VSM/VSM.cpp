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
float* Ex(float* dataDepth, half* hitMiss, int filterSize);
float Mean(float buffer[7][7], int hit);

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
	//stbi_write_png("OutputDepthSquare.png", width, height, 4, squareDepth, 4 * width);
	printf(" first element of squareDepth: %6f\n", *(squareDepth));
	printf(" first element of squareDepth: %6f\n", *(floatDepth + 91557));
	printf(" first element of squareDepth: %6f\n", *(squareDepth + 91557));

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
		intPointer[i] = (__int8)(*(floatPointer + i));
	}
	return intPointer;
}

float* Ex(float* dataDepth, half* hitMiss, int filterSize) {

	int hit = 0;
	float buffer[7][7];
	float* mean;

	for (int h = 0; h < height - 7 + 1; h++) {

		for (int w = 0; w < width - 7 + 1; w++) {

			for (int j = 0; j < 7; j++) {

				for (int k = 0; k < 7; k++) {

					if (HalfToFloat(*(hitMiss + (4 * (k + w)) + ((j + h)*width) + 3)) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. ist der Hit/Miss-Wert.
						hit = hit + 1;
						buffer[j][k] = *(dataDepth + (k + w) + ((j + h)*width));
					}
					else {
						buffer[j][k] = 0;
					}
				}
			}
			mean[((3+h)*width)+3+w] = Mean(buffer, hit);
			hit = 0;
		}
	}
}

float Mean(float buffer[7][7], int hit) {

	float meanValue = 0.0;

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			meanValue = meanValue + (buffer[i][j] * (1 / hit));
		}
	}
	return meanValue;
}