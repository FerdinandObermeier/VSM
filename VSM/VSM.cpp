// VSM.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <cstdio>
#include <cassert>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

using byte = unsigned char;

byte* readIn(int width, int height, int byteSize, string filename);
byte* makeSquareDepth(byte* depth);


int main()
{
	
	int width = 1280;
	int height = 720;
	int byteSizeDepth = 4;
	int byteSizeColor = 16;
	string filenameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	string filenameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	byte* depth= readIn(width, height, byteSizeDepth, filenameDepth);
	byte* color = readIn(width, height, byteSizeColor, filenameColor);
	stbi_write_png("Output.png", width, height, 4, color, 16);
	int test = *(color + (91557 * 16));
	printf(" first element: %i\n", test);
	byte* squareDepth = makeSquareDepth(depth);
	printf(" first element: %i\n", *(squareDepth + 2));
	
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

byte* makeSquareDepth(byte* depth)//quadriert Zelleninhalte des Arrays bei depth
{
	
	byte* squareDepth = new byte[1024*768*4];
	for (int i = 0; i < (1024 * 768 * 4); i++) {
		squareDepth[i] = (*depth)*(*depth);
		depth++;
	}
	return squareDepth;
}
