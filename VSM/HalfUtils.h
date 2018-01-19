#ifndef __HALFUTILS_H__
#define __HALFUTILS_H__

#include <cstring> // for memcpy

typedef short half;

inline half FloatToHalf( float value )
{
	half fltInt16;
	int fltInt32;
	memcpy(&fltInt32, &value, sizeof(float));

	fltInt16 = ((fltInt32 & 0x7fffffff) >> 13) - (0x38000000 >> 13);
	fltInt16 |= ((fltInt32 & 0x80000000) >> 16);

	return fltInt16;
}

inline float HalfToFloat(half value)
{
	int fltInt32 = ((value & 0x8000) << 16);
	fltInt32 |= ((value & 0x7fff) << 13) + 0x38000000;

	float fRet;
	memcpy(&fRet, &fltInt32, sizeof(float));
	return fRet;
}



#endif /* __HALFUTILS_H__ */