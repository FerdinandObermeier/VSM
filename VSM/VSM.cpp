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

byte* readIn(int width, int height, int byteSize, string filename);
float* makeSquareDepth(float* depth);
float* byteToFloatPointer(byte* bytePointer);

int main()
{
	
	int width = 1280;
	int height = 720;
	int byteSizeDepth = 4;
	int byteSizeColor = 16;
	float* floatdepth;
	string filenameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	string filenameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	byte* depth= readIn(width, height, byteSizeDepth, filenameDepth);
	byte* color = (readIn(width, height, byteSizeColor, filenameColor));
	//floatcolor = (float*)color;
	stbi_write_png("Output.png", width, height, 4, color, 16);
	int test = *(color + (91557 * 16));
	printf(" 91557. element of color: %i\n", test);
	floatdepth = byteToFloatPointer(depth);
	printf(" first element of floatdepth: %6f\n", *(floatdepth));
	float* squareDepth = makeSquareDepth(floatdepth);
	printf(" first element of squareDepth: %6f\n", *(squareDepth));
	
	/*byte test[3] = { 1,2,3 };
	byte* name = test;
	byte* squareDepth = makeSquareDepth(name);
	printf(" first element: %i\n", *(squareDepth++));*/
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

float* makeSquareDepth(float* depth)//quadriert Zelleninhalte des Arrays bei depth
{
	
	float* squareDepth = new float[1024*768];
	for (int i = 0; i < (1024 * 768); i++) {
		squareDepth[i] = (*depth)*(*depth);
		depth++;
	}
	return squareDepth;
}

float* byteToFloatPointer(byte* bytePointer) {

	float buffer;
	byte bufferArray[4];
	float* floatPointer = new float[sizeof(bytePointer)/4];

	for (int i = 0; i < (sizeof(bytePointer)); i=i + 4) {
		bufferArray[3] = *(bytePointer + i);
		bufferArray[2] = *(bytePointer + i + 1);
		bufferArray[1] = *(bytePointer + i + 2);
		bufferArray[0] = *(bytePointer + i + 3);
		memcpy(&buffer, &bufferArray, sizeof(buffer));
		floatPointer[i] = buffer;
	}
	return floatPointer;
}