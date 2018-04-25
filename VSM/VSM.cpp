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
#include <tuple>
#include <time.h>
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
__int8* depthToRGBA(float* depth);
double* floatToDouble(float* floatPointer);
tuple<double*, double*, double*> filter(double* depth, double* squareDepth, half* hitMiss, size_t size, bool withoutMax);
tuple<double, double> WeightedMean(double* buffer, double* squaredBuffer, int hitValue, double minDepth, double maxDepth, double minSquareDepth, double maxSquareDepth, size_t size);
tuple<double, double> Threshold(double* buffer, double* squaredBuffer, int hitValue, size_t size);
tuple<double, double> calculateMean(double*buffer, double* squaredBuffer, int hitValue, size_t size, bool withoutMax);
tuple<double,double> minMaxDepthD(double* depth, half* hitMiss);
double calculateVariance(double ex, double ex_2);
float maxValueF(float* data);
double maxValueD(double* data);
float* convertVtoHue(double* variance);
__int8* convertHSLtoRGB(float* hue);
float* newDepth(double* variance, double* depth, double* ex);
__int8* Normals(float* depth, half* hitMiss);
vector<float> Pos3D(vector<int> position, vector<int> offset, float depth);
__int8* Normals3D(float* depth, half* hitMiss);
tuple<int, int, bool, string> prepareForSmoothing();
void smooth(float* floatColor, float* floatDepth, double* oldDepth, double* oldSquaredDepth, half* hitMiss, int iterations, size_t size, bool withoutMax, string name);


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
	tuple<int, int, bool, string> smoothingParams;

/*******************************************  INPUT FILES  ********************************************************************/

	string fileNameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	string fileNameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	string fileNameNormals = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Normals.raw";

	//string fileNameDepth = "Dragon_Spheres_1.5/framedump_09012018_115927_Depth.raw";
	//string fileNameColor = "Dragon_Spheres_1.5/framedump_09012018_115927_Colors.raw";
	//string fileNameNormals = "Dragon_Spheres_1.5/framedump_09012018_115927_Normals.raw";

	//string fileNameDepth = "Dragon_Spheres_1.0/framedump_09012018_115957_Depth.raw";
	//string fileNameColor = "Dragon_Spheres_1.0/framedump_09012018_115957_Colors.raw";
	//string fileNameNormals = "Dragon_Spheres_1.0/framedump_09012018_115957_Normals.raw";

	//string fileNameDepth = "Audimax_Spheres_1.5/framedump_09012018_141355_Depth.raw";
	//string fileNameColor = "Audimax_Spheres_1.5/framedump_09012018_141355_Colors.raw";
	//string fileNameNormals = "Audimax_Spheres_1.5/framedump_09012018_141355_Normals.raw";


/******************************************************************************************************************************/


	byte* depth = readIn(width, height, byteSizeDepth, fileNameDepth);			//
	byte* color = readIn(width, height, byteSizeColor, fileNameColor);			//Lese .raw Dateien ein und speichere sie als Pointer
	byte* normals = readIn(width, height, byteSizeNormals, fileNameNormals);	//

	floatColor = (float*)color;						//
	floatDepth = (float*)depth;						//
	doubleDepth = floatToDouble(floatDepth);		//Anpassung der Typen
	squaredDepth = makeSquareDepth(floatDepth);		//
	halfNormals = (half*)normals;					//
	normalsHitMiss = (half*)normals;				//

	__int8* intcolor = depthToRGBA(floatDepth);
	stbi_write_png("Depth.png", width, height, 1, intcolor, width);


	smoothingParams = prepareForSmoothing();     //der User gibt die Parameterwerte ein

	smooth(floatColor, floatDepth, doubleDepth, squaredDepth, halfNormals, get<0>(smoothingParams), get<1>(smoothingParams), get<2>(smoothingParams), get<3>(smoothingParams));

	delete depth, color, normals, floatColor, floatDepth, doubleDepth, squaredDepth, halfNormals, normalsHitMiss;
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
		squareDepth[i] = (depth[i])*(depth[i]);
		if (isinf(squareDepth[i])) {
			squareDepth[i] = DBL_MAX;
		}
	}
	return squareDepth;
}

__int8* floatToRGBA(float* floatPointer) {//konvertiert float Values zu int Werten zwischen 0 und 255 (RGBA)

	__int8* intPointer = new __int8[width*height*4];

	for (int i = 0; i < width*height*4; i++) {
		intPointer[i] = (__int8)((floatPointer[i]) * 255);
	}
	return intPointer;
}


__int8* depthToRGBA(float* depth) {

	__int8* intPointer = new __int8[width*height];
	float max = 0;


	for (int i = 0; i < width*height; i++) {
		if (depth[i] > max && depth[i]!=FLT_MAX && HalfToFloat(normalsHitMiss[4*i+3])==1) {
			max = depth[i];
		}
	}

	for (int i = 0; i < width*height; i++) {
		if (HalfToFloat(normalsHitMiss[4 * i + 3]) == 1) {
			intPointer[i] = (__int8)(((depth[i] / max) * 255));
		}
		else {
			intPointer[i] = 0;
		}
	}

	return intPointer;
}

double* floatToDouble(float* floatPointer) {//macht aus einem float Pointer einen Double Pointer

	double* doublePointer = new double[width*height];

	for (int i = 0; i < width*height; i++) {
		doublePointer[i] = (double)(floatPointer[i]);
	}
	return doublePointer;
}



tuple<double*, double*, double*> filter(double* depth, double* squareDepth, half* hitMiss, size_t size, bool withoutMax) {//berechnet den Erwartungswert von depth und squareDepth mit einem size*size Filter


	double* buffer = new double[size*size];
	double* squareBuffer = new double[size*size];
	int hit = 0;
	double* mean = new double[width*height];
	double* squareMean = new double[width*height];
	double* variance = new double[width*height];
	tuple<double, double> minMaxDepth = minMaxDepthD(depth, hitMiss);
	tuple<double, double> MinMaxSquareDepth = minMaxDepthD(squareDepth, hitMiss);
	tuple<double, double> MeanBuffer;


	for (int h = 0; h < height; h++) {
		if (h < size / 2 || h >= (height - size / 2)) {//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
				squareMean[m + (h * width)] = -1.0;
			}
		}
		else {
			for (int w = 0; w < width; w++) {//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt
				if (w < size / 2 || w >= (width - size / 2)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = -1.0;
						squareMean[w + (n * width)] = -1.0;
					}
				}
				else {
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {//Wenn kein Miss, dann wird das Pixel genauer betrachtet

						for (int j = 0; j < size; j++) {

							for (int k = 0; k < size; k++) {

								if (HalfToFloat(hitMiss[((4 * ((k + (w - size / 2)) + ((j + (h - size / 2))*width))) + 3)]) == 1.0) {//4* wegen RGBA also 4 half Werte. +3 weil immer der 3. Wert der Hit/Miss-Wert ist.
									hit = hit + 1;
									buffer[j * size + k] = depth[(k + (w - size / 2)) + ((j + (h - size / 2))*width)];
									squareBuffer[j * size + k] = squareDepth[(k + (w - size / 2)) + ((j + (h - size / 2))*width)];
								}
								else {
									buffer[j*size + k] = 0.0;
									squareBuffer[j*size + k] = 0.0;
								}
							}
						}
						if (hit > ((size*size)/2)) {//Wenn mindestens die Hälfte der Pixel im Filter "Hits" sind, wird ein Erwartungswert berechnet. Dadurch werden Löcher im Drachen gefüllt.
							if (true) {
								MeanBuffer = Threshold(buffer, squareBuffer, hit, size);
								mean[(h*width) + w] = get<0>(MeanBuffer);
								squareMean[(h*width) + w] = get<1>(MeanBuffer);
								variance[(h*width) + w] = calculateVariance(mean[(h*width) + w], squareMean[(h*width) + w]);

								/*MeanBuffer = WeightedMean(buffer, squareBuffer, hit, get<0>(minMaxDepth), get<1>(minMaxDepth), get<0>(MinMaxSquareDepth), get<1>(MinMaxSquareDepth), size);
								mean[(h*width) + w] = get<0>(MeanBuffer);
								squareMean[(h*width) + w] = get<1>(MeanBuffer);
								variance[(h*width) + w] = calculateVariance(mean[(h*width) + w], squareMean[(h*width) + w]);*/
							}
							else {
								MeanBuffer = calculateMean(buffer, squareBuffer, hit, size, withoutMax);
								mean[(h*width) + w] = get<0>(MeanBuffer);
								squareMean[(h*width) + w] = get<1>(MeanBuffer);
								variance[(h*width) + w] = calculateVariance(mean[(h*width) + w], squareMean[(h*width) + w]);
							}
						}
						else {
							variance[(h*width) + w] = 0;
						}
					}
					else {
						variance[(h*width) + w] = 0;
					}
				}
				hit = 0;
			}
		}
	}
	delete buffer;
	delete squareBuffer;
	return make_tuple(mean, squareMean, variance);
	delete mean;
	delete squareMean;
}

//Berechnet den Erwartungswert der Tiefenwerte und der quadrierten Tiefenwerte einer Verteilung
//Gewichtet dabei die Tiefenwerte, wobei kleine Tiefenwerte stärker gewichtet werden als große

tuple<double, double> WeightedMean(double* buffer, double* squaredBuffer, int hitValue, double minDepth, double maxDepth, double minSquareDepth, double maxSquareDepth, size_t size) {//Berechnet den gewichteten Erwartungswert von size*size Werten

	double average = 0.0;
	double meanValue = 0.0;
	double meanSquaredValue = 0.0;
	double localMax = 0.0;
	double localMax2 = 0.0;

	for (int a = 0; a < size*size; a++) {
		if (buffer[a] > localMax) {
			localMax2 = localMax;
			localMax = buffer[a];
		}
		else if (buffer[a] > localMax2) {
			localMax2 = buffer[a];
		}
		average += buffer[a] * (1.0 / (double)hitValue);
	}

	if (localMax - localMax2 >= 0.2*average&&false) {
		hitValue -= 1;
	}

	for (int i = 0; i < size*size; i++) {
		if (buffer[i]<localMax&&false) {
			meanValue +=  ((buffer[i] * (1 - (buffer[i] - minDepth) / maxDepth)) * (1.0 / (double)hitValue));
			meanSquaredValue +=  ((squaredBuffer[i] * (1 - (squaredBuffer[i] - minSquareDepth) / maxSquareDepth)) * (1.0 / (double)hitValue));
		}
		else if (localMax - localMax2 < 0.2*average||true) {
			meanValue += ((buffer[i] * (1 - (buffer[i] - minDepth) / maxDepth)) * (1.0 / (double)hitValue));
			meanSquaredValue += ((squaredBuffer[i] * (1 - (squaredBuffer[i] - minSquareDepth) / maxSquareDepth)) * (1.0 / (double)hitValue));
		}
	}
	return make_tuple(meanValue, meanSquaredValue);

}


//Berechnet den Erwartungswert der Tiefenwerte und der quadrierten Tiefenwerte einer Verteilung
//Vernachlässigt dabei alle Tiefenwerte die größer sind als ein bestimmter Threshold

tuple<double, double> Threshold(double* buffer, double* squaredBuffer, int hitValue, size_t size) {//Berechnet den gewichteten Erwartungswert von size*size Werten

	double average = 0.0;
	double meanValue = 0.0;
	double meanSquaredValue = 0.0;
	double localMin = DBL_MAX;
	double threshold = 0;


	for (int a = 0; a < size*size; a++) {
		average += buffer[a] * (1.0 / (double)hitValue);
		if (buffer[a] < localMin) {
			localMin = buffer[a];
		}
	}

	threshold = 1.02*average;

	for (int b = 0; b < size*size; b++) {
		if (buffer[b] > threshold) {
			hitValue--;
		}
	}


	for (int i = 0; i < size*size; i++) {
		if (buffer[i]<= threshold) {
			meanValue += (buffer[i] * (1.0 / (double)hitValue));
			meanSquaredValue += (squaredBuffer[i] * (1.0 / (double)hitValue));
		}
	}

	return make_tuple(meanValue, meanSquaredValue);

}


//Berechnet den Erwartungswert der Tiefenwerte und der quadrierten Tiefenwerte einer Verteilung
//Vernachlässigt das lokale Maximum, wenn withoutMax true ist

tuple<double, double> calculateMean(double*buffer, double* squaredBuffer, int hitValue, size_t size, bool withoutMax) {//Berechnet den Erwartungswert mit oder ohne (je nach User Input) lokalem Maximum für depth und squareDepth

	double meanValue = 0.0;
	double meanSquaredValue = 0.0;
	double localMax = 0.0;
	double localMin = 100.0;
	bool unique = false;
	
	if (withoutMax) {
	for (int i = 0; i < size*size; i++) {
		if (buffer[i] > localMax) {
			localMax = buffer[i];
		}
		if (buffer[i] < localMin) {
			localMin = buffer[i];
		}
	}
		hitValue -= 1;
	}
	 

	for (int i = 0; i < size*size; i++) {
		if (buffer[i] != localMax || unique || !withoutMax) {
			meanValue += (buffer[i] * (1.0 / (double)hitValue));
			meanSquaredValue += (squaredBuffer[i] * (1.0 / (double)hitValue));
		}
		else {
			unique = !unique;
		}
	}

	return make_tuple(meanValue, meanSquaredValue);
}

double calculateVariance(double ex, double ex_2) {//Berechnet die Varianz aus Erwartungswert(x) und Erwartungswert(x^2)

	double variance = 0;

	variance = ex_2 - (ex*ex);
	if (isinf(variance)) {
		variance = DBL_MAX;
	}

	return variance;
}

tuple<double, double> minMaxDepthD(double* depth, half* hitMiss) {//gibt den minimalen und den maximalen Tiefenwert als Double in einem Tupel zurück

	double maxDepth = 0;
	double minDepth = 0;

	for (int i = 0; i < width*height; i++) {
		if (depth[i] > maxDepth && HalfToFloat(*(hitMiss + 4 * i + 3)) == 1.0) {
			maxDepth = depth[i];
		}
		if (depth[i] < minDepth && HalfToFloat(*(hitMiss + 4 * i + 3)) == 1.0) {
			minDepth = depth[i];
		}
	}
	return make_tuple(minDepth, maxDepth);
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

float* newDepth(double* variance, double* depth, double* ex) {//Berechnet den neuen Tiefenwert um Löcher zu schließen und zu glätten


	float* newDepth = new float[width*height];
	double p;

	for (int i = 0; i < width*height; i++) {
		if ((variance[i]) > 0) {
			/*p = variance[i] / (variance[i] + ((depth[i] -ex[i])*(depth[i] - ex[i])));
			if (p > 0.7) {
				newDepth[i] = p*depth[i];
			}
			else {
				newDepth[i] = depth[i];
			}*/
			newDepth[i] = ex[i];
			::normalsHitMiss[4 * i + 3] = FloatToHalf(1.0);
		}
		else {
			newDepth[i] = depth[i];
		}
	}
	return newDepth;
	delete newDepth;
}

float* convertVtoHue(double* variance) {//Konvertiert die Varianzwerte in HUE-Werte

	double max = maxValueD(variance);
	float* hue = new float[width*height];


	for (int i = 0; i < width*height; i++) {
		hue[i] = fabs((((variance[i]) / max)*240.0) - 240.0);//komprimiert die Varianz auf Werte zwischen [0,240] und kehrt den Wertebereich um
	}

	return hue;
	delete hue;
}

__int8* convertHSLtoRGB(float* hue) {//Konvertiert die von convertVtoHue berechneten HUE Werte mit festen Saturation- und Lightness-Werten zu RGB-Werten für stbi_write_png

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
	delete rgb;
}

__int8* Normals(float* depth, half* hitMiss) {//Berechnet die Normalen

	int j;
	float length;
	float* normals = new float[width*height * 3];
	__int8* normalsRGB = new __int8[width*height * 3];

	for (int i = 0; i < width*(height - 1); i++) {
		j = i * 3;

		if (HalfToFloat(hitMiss[4 * i + 3]) == 1.0) {
			normals[j] = (depth[i + 1] - depth[i]);
			normals[j + 1] = ((depth[i + 1280]) - (depth[i]));
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
	delete normalsRGB;
}

vector<float> Pos3D(vector<int> position, vector<int> offset, float depth) {

	float scaleY = tan(FOVY / 2.0f);
	float scaleX = scaleY * width / height;

	vector<float> pos (3);

	pos[0] = ((float(position[0] + offset[0]) / float(width))*2.0f - 1.0f)*scaleX*depth;
	pos[1] = ((float(position[1] + offset[1]) / float(height))*2.0f - 1.0f)*(-scaleY)*depth;
	pos[2] = depth;

	return pos;
}

__int8* Normals3D(float* depth, half* hitMiss) {//Berechnet die 3D Normalen mit Hilfe von Pos3D()

	int k;
	vector<int> pos (2), offset (2);
	vector<float> a (3), b (3), c (3), d (3), e (3);
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
	delete normals3DRGB;
}


tuple<int, int, bool, string> prepareForSmoothing() {//Erfragt User Input für die Parameter Iterations (Anzahl Iterationen), filterSize (Filtergröße), withoutMax (Betrachtung des lokalen Maximums  JA/NEIN) und name (Name der Ausgabedateien)


	int iterations;
	int filterSize;
	int ohneMax;
	bool withoutMax;
	string name;
	cout << "(1/4) Geben Sie die gew\201nschte Anzahl an Iterationen ein: ";
	cin >> iterations;
	while (cin.fail() || iterations == 0 || iterations > 40) {
		cout << "Die Eingabe ist ung\201ltig. Es sind nur Integer ungleich 0 und kleiner gleich 40 zul\204ssig. Bitte versuchen Sie es erneut: ";
		cin >> iterations;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << iterations << " wurde als Anzahl an Iterationen erkannt.\n";
	cout << "(2/4) Geben Sie nun die gew\201nschte Filtergr\224\341e ein (z.B. 3 f\201r 3*3): ";
	cin >> filterSize;
	while (filterSize % 2 == 0 || filterSize < 3 || cin.fail()) {
		cout << "Die Eingabe ist ung\201ltig. Es sind nur ungerade Zahlen gr\224\341er gleich 3 zul\204ssig. Bitte versuchen Sie es erneut: ";
		cin >> filterSize;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << filterSize << " wurde als Filtergr\224\341e gew\204hlt.\n";
	cout << "(3/4) Soll der maximale Wert mitbetrachtet werden (1) oder nicht(0)? ";
	cin >> ohneMax;
	while (cin.fail() || ohneMax < 0 || ohneMax > 1) {
		cout << "Die Eingabe ist ung\201ltig. Versuchen Sie es erneut: ";
		cin >> ohneMax;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	if (ohneMax == 0) {
		withoutMax = true;
		cout << "Der maximale Wert wird nicht mitbetrachtet.\n";
	}
	else {
		withoutMax = false;
		cout << "Der maximale Wert wird mitbetrachtet.\n";	
	}

	cout << "(4/4) Wie sollen die Ausgabedateien hei\341en (ohne Leerzeichen)? ";
	cin >> name;
	while (cin.fail()) {
		cout << "Die Eingabe ist ung\201ltig. Versuchen Sie es erneut: ";
		cin >> name;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << "Ihre Ausgabedateien hei\341en: " << name << "_0 - " << name << "_" << iterations << " .png\n\n";


	return make_tuple(iterations, filterSize, withoutMax, name);
}


void smooth(float* floatColor, float* floatDepth, double* oldDepth, double* oldSquaredDepth, half* hitMiss, int iterations, size_t size, bool withoutMax, string name) {//Führt iterations Glättungsschritte aus und speichert den Output von 0-iterations in name

	cout << "Iteration 1 von " << iterations << " startet jetzt.\n";

	tuple<double*, double*, double*> buffer = filter(oldDepth, oldSquaredDepth, hitMiss, size, withoutMax);
	double* oldMean = get<0>(buffer);
	double* oldMeanSquared = get<1>(buffer);
	double* oldVariance = get<2>(buffer);
	float* hue = convertVtoHue(oldVariance);
	float* FloatNewDepth = new float[width*height];
	clock_t t1;
	clock_t t2;

	__int8* intcolor = floatToRGBA(floatColor);
	stbi_write_png("Colors.png", width, height, 4, intcolor, 4 * width);

	__int8* normals3D = Normals3D(floatDepth, hitMiss);
	stbi_write_png("Normalen3D.png", width, height, 3, normals3D, 3 * width);

	__int8* normalsRGB = Normals(floatDepth, hitMiss);
	stbi_write_png("Normalen.png", width, height, 3, normalsRGB, 3 * width);
	
	__int8* varianceRGB = convertHSLtoRGB(hue);
	stbi_write_png("VarianzRGB.png", width, height, 3, varianceRGB, 3 * width);
	
	t1 = clock();
	for (int i = 0; i < iterations; i++) {

		string s = "NeueGewichteteVarianz_" + to_string(i) + ".png";
		const char* c = s.c_str();

		string r = name + "_" + to_string(i) + ".png";
		const char* d = r.c_str();


		float* newFloatDepth = newDepth(oldVariance, oldDepth, oldMean);
		double* newSquaredDepth = makeSquareDepth(newFloatDepth);
		double* newDoubleDepth = floatToDouble(newFloatDepth);
		buffer = filter(newDoubleDepth, newSquaredDepth, hitMiss, size, withoutMax);
		double* newMean = get<0>(buffer);
		double* newSquaredMean = get<1>(buffer);
		double* newVariance = get<2>(buffer);
		float* hue = convertVtoHue(newVariance);


		__int8* newDepthRGB = convertHSLtoRGB(hue);
		stbi_write_png(c, width, height, 3, newDepthRGB, 3 * width);

		__int8* newNormalsRGB = Normals3D(newFloatDepth, normalsHitMiss);
		stbi_write_png(d, width, height, 3, newNormalsRGB, 3 * width);

		oldDepth = newDoubleDepth;
		oldVariance = newVariance;
		oldMean = newMean;
		hitMiss = normalsHitMiss;


		cout <<"Iteration " << i+1 << " von " << iterations << " ist abgeschlossen.\n";

		t2 = clock()-t1;
		printf("Die Berechnung dauerte %d clicks (%f Sekunden).\n", t2, ((float)t2) / CLOCKS_PER_SEC);


		FloatNewDepth = newFloatDepth;
	}

	__int8* intdepth = depthToRGBA(FloatNewDepth);
	stbi_write_png("SmoothedDepth.png", width, height, 1, intdepth, width);

}



