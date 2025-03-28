#pragma once
struct IntVec4
{
public:
	int x = 0;
	int y = 0;
	int z = 0;
	int w = 0;

public:
	~IntVec4() = default;
	IntVec4() = default;
	IntVec4(const IntVec4& copyFrom);
	explicit IntVec4(int initialX, int initialY, int initialZ, int initialW);

	static IntVec4 ZERO;

	void SetFromText(char const* text);

	// Operators (const)
	bool		operator==(const IntVec4& compare) const;
	bool		operator!=(const IntVec4& compare) const;
	const IntVec4	operator+(const IntVec4& vecToAdd) const;
	const IntVec4	operator-(const IntVec4& vecToSubtract) const;
	const IntVec4	operator-() const;
	const IntVec4	operator*(int uniformScale) const;
	const IntVec4	operator*(const IntVec4& vecToMultiply) const;
	const IntVec4	operator/(int inverseScale) const;

	// Operators (self-mutating / non-const)
	void		operator+=(const IntVec4& vecToAdd);
	void		operator-=(const IntVec4& vecToSubtract);
	void		operator*=(const int uniformScale);
	void		operator/=(const int uniformDivisor);
	void		operator=(const IntVec4& copyFrom);

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	friend const IntVec4 operator*(int uniformScale, const IntVec4& vecToScale);
};