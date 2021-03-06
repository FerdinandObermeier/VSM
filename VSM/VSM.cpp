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

byte* readIn(int width, int height, int byteSize, string filename);
double* makeSquareDepth(float* depth);
__int8* floatToRGBA(float* floatPointer);
__int8* depthToRGBA(float* depth);
double* floatToDouble(float* floatPointer);
tuple<double*, double*, double*> filter(double* depth, double* squareDepth, half* hitMiss, size_t size, bool threshold, bool withoutMax);
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
tuple<int, int, bool, bool, string> prepareForSmoothing();
void smooth(float* floatColor, float* floatDepth, double* oldDepth, double* oldSquaredDepth, half* hitMiss, int iterations, size_t size, bool threshold, bool withoutMax, string name);


/******************GLOBALE VARIABLEN*****************/
const int width = 1280;
const int height = 720;
const float FOVY = 0.785398163f;
double thresholdMultiply;
half* normalsHitMiss = new half[width*height * 4];
/****************************************************/

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
	tuple<int, int, bool, bool, string> smoothingParams;

/*******************************************  INPUT FILES  ********************************************************************/

	//string fileNameDepth = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Depth.raw";
	//string fileNameColor = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Colors.raw";
	//string fileNameNormals = "Dragon_Spheres_1.0_Jitter/framedump_09012018_120119_Normals.raw";

	//string fileNameDepth = "Dragon_Spheres_1.5/framedump_09012018_115927_Depth.raw";
	//string fileNameColor = "Dragon_Spheres_1.5/framedump_09012018_115927_Colors.raw";
	//string fileNameNormals = "Dragon_Spheres_1.5/framedump_09012018_115927_Normals.raw";

	//string fileNameDepth = "Dragon_Spheres_1.0/framedump_09012018_115957_Depth.raw";
	//string fileNameColor = "Dragon_Spheres_1.0/framedump_09012018_115957_Colors.raw";
	//string fileNameNormals = "Dragon_Spheres_1.0/framedump_09012018_115957_Normals.raw";

	//string fileNameDepth = "Audimax_Spheres_1.5/framedump_09012018_141355_Depth.raw";
	//string fileNameColor = "Audimax_Spheres_1.5/framedump_09012018_141355_Colors.raw";
	//string fileNameNormals = "Audimax_Spheres_1.5/framedump_09012018_141355_Normals.raw";

	string fileNameDepth = "Office_Spheres_1.5/framedump_09012018_134049_Depth.raw";
	string fileNameColor = "Office_Spheres_1.5/framedump_09012018_134049_Colors.raw";
	string fileNameNormals = "Office_Spheres_1.5/framedump_09012018_134049_Normals.raw";


/******************************************************************************************************************************/

	//Lese .raw Dateien ein und speichere sie als Pointer
	byte* depth = readIn(width, height, byteSizeDepth, fileNameDepth);
	byte* color = readIn(width, height, byteSizeColor, fileNameColor);
	byte* normals = readIn(width, height, byteSizeNormals, fileNameNormals);

	//Type casts
	floatColor = (float*)color;
	floatDepth = (float*)depth;
	doubleDepth = floatToDouble(floatDepth);
	squaredDepth = makeSquareDepth(floatDepth);
	halfNormals = (half*)normals;
	normalsHitMiss = (half*)normals;

	//Der User gibt die Parameterwerte ein
	smoothingParams = prepareForSmoothing();     

	//Der Glättungsprozess beginnt
	smooth(floatColor, floatDepth, doubleDepth, squaredDepth, halfNormals, get<0>(smoothingParams), get<1>(smoothingParams), get<2>(smoothingParams), get<3>(smoothingParams), get <4>(smoothingParams));

	delete depth, color, normals, floatColor, floatDepth, doubleDepth, squaredDepth, halfNormals, normalsHitMiss;
}


//liest Datei ein und speichert sie als byte-Pointer "rawData"

byte* readIn(int width, int height, int byteSize, string filename)
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

//quadriert die Tiefenwerte und gibt die quadrierten Tiefenwerte zurück

double* makeSquareDepth(float* depth)
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

//konvertiert float Values zu int Werten zwischen 0 und 255 (RGBA)

__int8* floatToRGBA(float* floatPointer) {

	__int8* intPointer = new __int8[width*height*4];

	for (int i = 0; i < width*height*4; i++) {
		intPointer[i] = (__int8)((floatPointer[i]) * 255);
	}
	return intPointer;
}

//macht das selbe wie floatToRGBA aber für weniger Werte

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

//macht aus einem float Pointer einen Double Pointer

double* floatToDouble(float* floatPointer) {

	double* doublePointer = new double[width*height];

	for (int i = 0; i < width*height; i++) {
		doublePointer[i] = (double)(floatPointer[i]);
	}
	return doublePointer;
}

//Iteriert über jedes Pixel und gibt am Ende für jedes Pixel den Erwartungswert der Tiefenwerte, den Erwartungswert der quadrierten Tiefenwerte und die Varianz zurück
//Für die Pixel an den äußeren Rändern und im Hintergrund werden Erwartungswert und Varianz auf fixe Werte gesetzt

tuple<double*, double*, double*> filter(double* depth, double* squareDepth, half* hitMiss, size_t size, bool threshold, bool withoutMax) {


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
		//Die Pixel am oberen und unteren Rand des Bildes werden auf fixe Werte gesetzt
		if (h < size / 2 || h >= (height - size / 2)) {
			for (int m = 0; m < width; m++) {
				mean[m + (h * width)] = -1.0;
				squareMean[m + (h * width)] = -1.0;
			}
		}
		else {
			//Die Pixel am linken und rechten Rand des Bildes werden auf fixe Werte gesetzt
			for (int w = 0; w < width; w++) {
				if (w < size / 2 || w >= (width - size / 2)) {
					for (int n = 0; n < height; n++) {
						mean[w + (n * width)] = -1.0;
						squareMean[w + (n * width)] = -1.0;
					}
				}
				else {
					//Wenn kein Miss, dann wird das Pixel genauer betrachtet
					if (((HalfToFloat(*(hitMiss + (4 * ((h*width) + w)) + 3)))) != 0.0) {

						for (int j = 0; j < size; j++) {

							for (int k = 0; k < size; k++) {
								//Nur für die Pixel im Objekt (also Hit/Miss Wert = 1) wird der hitValue erhöht und der Tiefenwert sowie der quadrierte Tiefenwert in die jeweiligen Buffer geschrieben
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
						//Wenn mindestens die Hälfte der Pixel im Filter "Hits" sind, wird ein Erwartungswert berechnet. Dadurch werden Löcher im Drachen gefüllt.
						if (hit > ((size*size)/2)) {
							if (threshold) {
								MeanBuffer = Threshold(buffer, squareBuffer, hit, size);
								mean[(h*width) + w] = get<0>(MeanBuffer);
								squareMean[(h*width) + w] = get<1>(MeanBuffer);
								variance[(h*width) + w] = calculateVariance(mean[(h*width) + w], squareMean[(h*width) + w]);

								//MeanBuffer = WeightedMean(buffer, squareBuffer, hit, get<0>(minMaxDepth), get<1>(minMaxDepth), get<0>(MinMaxSquareDepth), get<1>(MinMaxSquareDepth), size);
								//mean[(h*width) + w] = get<0>(MeanBuffer);
								//squareMean[(h*width) + w] = get<1>(MeanBuffer);
								//variance[(h*width) + w] = calculateVariance(mean[(h*width) + w], squareMean[(h*width) + w]);
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

tuple<double, double> WeightedMean(double* buffer, double* squaredBuffer, int hitValue, double minDepth, double maxDepth, double minSquareDepth, double maxSquareDepth, size_t size) {

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

tuple<double, double> Threshold(double* buffer, double* squaredBuffer, int hitValue, size_t size) {

	double average = 0.0;
	double meanValue = 0.0;
	double meanSquaredValue = 0.0;
	double localMin = DBL_MAX;
	double threshold = 0;

	//Berechnet den Durchschnitt beziehungsweise Erwartungswert der Verteilung
	for (int a = 0; a < size*size; a++) {
		average += buffer[a] * (1.0 / (double)hitValue);
		if (buffer[a] < localMin) {
			localMin = buffer[a];
		}
	}

	//Legt den Threshold fest, mit dem die Werte verglichen werden
	threshold = (1+thresholdMultiply)*average;

	//Für alle Tiefenwerte die größer sind als der Threshold, wird der hitValue um 1 reduziert
	for (int b = 0; b < size*size; b++) {
		if (buffer[b] > threshold) {
			hitValue--;
		}
	}

	//Alle Werte die kleiner oder gleich dem Threshold sind, fließen in die Berechnung der Erwartungswerte mit ein. Alle anderen Werte werden vernachlässigt. (hitValue wurde bereits angepasst)
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
	bool unique = false;
	
	//Wenn das lokale Maximum vernachlässigt werden soll, wird es zunächst berechnet und dann wird der hitValue um 1 dekrementiert
	if (withoutMax) {
		for (int i = 0; i < size*size; i++) {
			if (buffer[i] > localMax) {
				localMax = buffer[i];
			}
		}
		hitValue -= 1;
	}
	 
	//Alle Werte bis auf das lokale Maximum fließen in die Berechnung mit ein. Falls es mehrere Werte gibt, die so groß sind wie das lokale Maximum, wird über den bool unique garantiert, dass nur 1 Wert vernachlässigt wird
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

//Berechnet die Varianz aus Erwartungswert(x) und Erwartungswert(x^2)

double calculateVariance(double ex, double ex_2) {

	double variance = 0;

	variance = ex_2 - (ex*ex);
	if (isinf(variance)) {
		variance = DBL_MAX;
	}

	return variance;
}

//Gibt den minimalen und den maximalen Tiefenwert als Double in einem Tupel zurück

tuple<double, double> minMaxDepthD(double* depth, half* hitMiss) {

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

//Gibt den maximalen Float Wert zurück

float maxValueF(float* data) {

	float maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}

//Gibt den maximalen double Wert zurück

double maxValueD(double* data) {

	double maxValue = 0.0;

	for (int i = 0; i < width*height; i++) {
		if (data[i] > maxValue) {
			maxValue = data[i];
		}
	}
	return maxValue;
}

//Berechnet den neuen Tiefenwert um Löcher zu schließen und zu glätten

float* newDepth(double* variance, double* depth, double* ex) {


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

//Konvertiert die Varianzwerte in HUE-Werte

float* convertVtoHue(double* variance) {

	double max = maxValueD(variance);
	float* hue = new float[width*height];


	for (int i = 0; i < width*height; i++) {
		hue[i] = fabs((((variance[i]) / max)*240.0) - 240.0);//komprimiert die Varianz auf Werte zwischen [0,240] und kehrt den Wertebereich um
	}

	return hue;
	delete hue;
}

//Konvertiert die von convertVtoHue berechneten HUE Werte mit festen Saturation- und Lightness-Werten zu RGB-Werten für stbi_write_png

__int8* convertHSLtoRGB(float* hue) {

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

//Berechnet die 2D Normalen

__int8* Normals(float* depth, half* hitMiss) {

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

//Wird zur Berechnung der 3D Normalen gebraucht

vector<float> Pos3D(vector<int> position, vector<int> offset, float depth) {

	float scaleY = tan(FOVY / 2.0f);
	float scaleX = scaleY * width / height;

	vector<float> pos (3);

	pos[0] = ((float(position[0] + offset[0]) / float(width))*2.0f - 1.0f)*scaleX*depth;
	pos[1] = ((float(position[1] + offset[1]) / float(height))*2.0f - 1.0f)*(-scaleY)*depth;
	pos[2] = depth;

	return pos;
}

//Berechnet die 3D Normalen mit Hilfe von Pos3D()

__int8* Normals3D(float* depth, half* hitMiss) {

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

//Erfragt User Input für die Parameter Iterations (Anzahl Iterationen), filterSize (Filtergröße), withoutMax (Betrachtung des lokalen Maximums  JA/NEIN) und name (Name der Ausgabedateien)

tuple<int, int, bool, bool, string> prepareForSmoothing() {


	int iterations;
	int filterSize;
	int mode;
	bool threshold;
	int multiply;
	int ohneMax;
	bool withoutMax;
	string name;
	cout << "(1/5) Geben Sie die gew\201nschte Anzahl an Iterationen ein: ";
	cin >> iterations;
	while (cin.fail() || iterations == 0 || iterations > 40) {
		cout << "Die Eingabe ist ung\201ltig. Es sind nur Integer ungleich 0 und kleiner gleich 40 zul\204ssig. Bitte versuchen Sie es erneut: ";
		cin >> iterations;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << iterations << " wurde als Anzahl an Iterationen erkannt.\n";
	cout << "(2/5) Geben Sie nun die gew\201nschte Filtergr\224\341e ein (z.B. 3 f\201r 3*3): ";
	cin >> filterSize;
	while (filterSize % 2 == 0 || filterSize < 3 || cin.fail()) {
		cout << "Die Eingabe ist ung\201ltig. Es sind nur ungerade Zahlen gr\224\341er gleich 3 zul\204ssig. Bitte versuchen Sie es erneut: ";
		cin >> filterSize;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << filterSize << " wurde als Filtergr\224\341e gew\204hlt.\n";
	cout << "(3/5) M\224chten Sie einen Threshold(1) festlegen oder normal(0) gl\204tten? ";
	cin >> mode;
	while (cin.fail() || mode < 0 || mode > 1) {
		cout << "Die Eingabe ist ung\201ltig. Versuchen Sie es erneut: ";
		cin >> mode;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	if (mode==1) {
		threshold = true;
		cout << "(4/5) Geben Sie den Threshold in Vielfachen des Durchschnitts der Verteilung an. (Bsp.: Eingabe 2 -> +2% -> 1.02*average): ";
		cin >> multiply;
		while (multiply <0 || cin.fail()) {
			cout << "Die Eingabe ist ung\201ltig. Es sind nur positive Integer zul\204ssig. Bitte versuchen Sie es erneut: ";
			cin >> multiply;
		}
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cout << "Der Threshold wird bei " << 1 + (multiply)*0.01 << "*average liegen.\n";
		thresholdMultiply = multiply*0.01;
	}
	else {
		cout << "Die Gl\204ttung erfolgt ohne Threshold.\n";
		threshold = false;
		cout << "(4/5) Soll der maximale Wert mitbetrachtet werden (1) oder nicht(0)? ";
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
	}
	

	cout << "(5/5) Wie sollen die Ausgabedateien hei\341en (ohne Leerzeichen)? ";
	cin >> name;
	while (cin.fail()) {
		cout << "Die Eingabe ist ung\201ltig. Versuchen Sie es erneut: ";
		cin >> name;
	}
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << "Die Ausgabedateien hei\341en: " << name << "_0.png - " << name << "_" << iterations << ".png\n\n";

	

	return make_tuple(iterations, filterSize, threshold, withoutMax, name);
}

//Führt iterations Glättungsschritte aus und speichert den Output von 0-iterations in name

void smooth(float* floatColor, float* floatDepth, double* oldDepth, double* oldSquaredDepth, half* hitMiss, int iterations, size_t size, bool threshold, bool withoutMax, string name) {

	cout << "Berechne Anfangsvarianz...\n";

	tuple<double*, double*, double*> buffer = filter(oldDepth, oldSquaredDepth, hitMiss, size, threshold, withoutMax);
	double* oldMean = get<0>(buffer);
	double* oldMeanSquared = get<1>(buffer);
	double* oldVariance = get<2>(buffer);
	float* hue = convertVtoHue(oldVariance);
	clock_t t1;
	clock_t t2;

	cout << "Erstelle Colors.png...\n";

	__int8* intcolor = floatToRGBA(floatColor);
	stbi_write_png("Colors.png", width, height, 4, intcolor, 4 * width);

	cout << "Erstelle 2D und 3D Normalen (Normalen.png, Normalen3D.png)...\n";

	__int8* normals3D = Normals3D(floatDepth, hitMiss);
	stbi_write_png("Normalen3D.png", width, height, 3, normals3D, 3 * width);

	__int8* normalsRGB = Normals(floatDepth, hitMiss);
	stbi_write_png("Normalen.png", width, height, 3, normalsRGB, 3 * width);
	
	cout << "Visualisiere Anfangsvarianz (VarianzRGB.png)...\n";

	__int8* varianceRGB = convertHSLtoRGB(hue);
	stbi_write_png("VarianzRGB.png", width, height, 3, varianceRGB, 3 * width);


	cout << "Starte Iteration 1 von " << iterations << ".\n";

	t1 = clock();
	for (int i = 0; i < iterations; i++) {

		string s = "NeueGewichteteVarianz_" + to_string(i) + ".png";
		const char* c = s.c_str();

		string r = name + "_" + to_string(i) + ".png";
		const char* d = r.c_str();


		float* newFloatDepth = newDepth(oldVariance, oldDepth, oldMean);
		double* newSquaredDepth = makeSquareDepth(newFloatDepth);
		double* newDoubleDepth = floatToDouble(newFloatDepth);
		buffer = filter(newDoubleDepth, newSquaredDepth, hitMiss, size, threshold, withoutMax);
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

		cout << "Starte Iteration " << i + 2 << " von " << iterations << ".\n";
	}
}



