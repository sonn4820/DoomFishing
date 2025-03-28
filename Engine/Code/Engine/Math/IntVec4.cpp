#include "Engine/Math/IntVec4.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <math.h>

IntVec4::IntVec4(const IntVec4& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
	w = copyFrom.w;
}

IntVec4::IntVec4(int initialX, int initialY, int initialZ, int initialW)
{
	x = initialX;
	y = initialY;
	z = initialZ;
	w = initialW;
}

IntVec4 IntVec4::ZERO = IntVec4(0, 0, 0, 0);

void IntVec4::SetFromText(char const* text)
{
	Strings strings = SplitStringOnDelimiter(text, ',');

	if (strings.size() != 4)
	{
		ERROR_AND_DIE("INPUT WRONG FORMAT INTVEC2");
	}

	const char* xChar = strings[0].c_str();
	const char* yChar = strings[1].c_str();
	const char* zChar = strings[2].c_str();
	const char* wChar = strings[3].c_str();

	x = atoi(xChar);
	y = atoi(yChar);
	z = atoi(zChar);
	w = atoi(wChar);
}

//-----------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator + (const IntVec4& vecToAdd) const
{
	return IntVec4(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z, w + vecToAdd.w);
}


//-----------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator-(const IntVec4& vecToSubtract) const
{
	return IntVec4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

//-----------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator-() const {
	return IntVec4(-x, -y, -z, -w);
}

//-----------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator*(int uniformScale) const
{
	return IntVec4(x * uniformScale, y * uniformScale, z * uniformScale, w * uniformScale);
}


//------------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator*(const IntVec4& vecToMultiply) const
{
	return IntVec4(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z, w * vecToMultiply.w);
}


//-----------------------------------------------------------------------------------------------
const IntVec4 IntVec4::operator/(int inverseScale) const
{
	float scale = 1.f / inverseScale;
	return IntVec4((int)(x * scale), (int)(y * scale), (int)(z * scale), (int)(w * scale));
}


//-----------------------------------------------------------------------------------------------
void IntVec4::operator+=(const IntVec4& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
	w += vecToAdd.w;
}


//-----------------------------------------------------------------------------------------------
void IntVec4::operator-=(const IntVec4& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
	w -= vecToSubtract.w;
}


//-----------------------------------------------------------------------------------------------
void IntVec4::operator*=(const int uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
	w *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void IntVec4::operator/=(const int uniformDivisor)
{
	float scale = 1.f / uniformDivisor;
	x = (int)((float)x * scale);
	y = (int)((float)y * scale);
	z = (int)((float)z * scale);
	w = (int)((float)w * scale);
}


//-----------------------------------------------------------------------------------------------
void IntVec4::operator=(const IntVec4& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
	w = copyFrom.w;
}


//-----------------------------------------------------------------------------------------------
const IntVec4 operator*(int uniformScale, const IntVec4& vecToScale)
{
	int x = vecToScale.x * uniformScale;
	int y = vecToScale.y * uniformScale;
	int z = vecToScale.z * uniformScale;
	int w = vecToScale.w * uniformScale;
	return IntVec4(x, y, z, w);
}


//-----------------------------------------------------------------------------------------------
bool IntVec4::operator==(const IntVec4& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z && w == compare.w;
}


//-----------------------------------------------------------------------------------------------
bool IntVec4::operator!=(const IntVec4& compare) const
{
	return x != compare.x || y != compare.y || z != compare.z || w != compare.w;
}
