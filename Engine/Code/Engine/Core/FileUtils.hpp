#pragma once
#include <vector>
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EventSystem.hpp"


int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& fileName);
int FileReadToString(std::string& outString, const std::string& fileName);
bool FileWriteFromBuffer(std::vector<uint8_t> const& buffer, std::string const& filePathName);
bool CreateFolder(std::string const& folderPathName);
bool HasFile(std::string const& folderPathName);
void WriteBufferToFile(std::vector<uint8_t> const& buffer, std::string const& filename);

bool LoadBinaryFileToExistingBuffer(std::string filePath, std::vector<unsigned char>& outBuffer);
bool SaveBinaryFileFromBuffer(std::string filePath, std::vector<unsigned char> inBuffer);

enum class EndianMode
{
	NATIVE,
	LITTLE,
	BIG
};

EndianMode GetPlatformLocalEndian();

class BufferWriter
{
public:
	BufferWriter(std::vector<uint8_t>& bufferToWrite);

	void AppendByte(uint8_t byteToAppend);
	void AppendChar(char charToAppend);
	void AppendUshort(unsigned short ushortToAppend);
	void AppendShort(signed short shortToAppend);
	void AppendUint32(unsigned int uintToAppend);
	void AppendInt32(signed int intToAppend);
	void AppendUint64(uint64_t uintToAppend);
	void AppendInt64(int64_t intToAppend);
	void AppendFloat(float floatToAppend);
	void AppendDouble(double doubleToAppend);
	void AppendBool(bool boolToAppend);
	void AppendZeroTerminatedString(std::string const& strToAppend);
	void AppendStringAfter32BitLength(std::string const& strToAppend);
	void AppendVec2(Vec2 const& vecToAppend);
	void AppendVec3(Vec3 const& vecToAppend);
	void AppendVec4(Vec4 const& vecToAppend);
	void AppendIntVec2(IntVec2 const& vecToAppend);
	void AppendIntVec3(IntVec3 const& vecToAppend);
	void AppendIntVec4(IntVec4 const& vecToAppend);
	void AppendRgba8(Rgba8 const colorToAppend);
	void AppendRgb8(Rgba8 const colorToAppend);
	void AppendAABB2(AABB2 const& aabb2ToAppend);
	void AppendAABB3(AABB3 const& aabb3ToAppend);
	void AppendOBB2(OBB2 const& obb2ToAppend);
	void AppendOBB3(OBB3 const& obb3ToAppend);
	void AppendPlane2(Plane2 const& plane2ToAppend);
	void AppendPlane3(Plane3 const& plane3ToAppend);
	void AppendVertexPCU(Vertex_PCU const& vertToAppend);
	void WriteIntToPos(size_t startPos, unsigned int intToWrite);
	void SetPreferredEndianMode(EndianMode mode);
	EndianMode GetPreferredEndianMode() const;
	EndianMode GetLocalEndianMode() const;
	size_t GetCurBufferSize() const;
protected:
	void SwitchByte(uint8_t* a, uint8_t* b) const;
	std::vector<uint8_t>& m_buffer;
	EndianMode m_localEndianMode;
	EndianMode m_preferredEndianMode = EndianMode::LITTLE;
};
class BufferParser
{
public:
	BufferParser(uint8_t const* const& buffer, size_t size);
	BufferParser(std::vector<uint8_t> const& bufferToRead);

	uint8_t ParseByte();
	char ParseChar();
	unsigned short ParseUshort();
	signed short ParseShort();
	unsigned int ParseUint32();
	signed int ParseInt32();
	uint64_t ParseUint64();
	int64_t ParseInt64();
	float ParseFloat();
	double ParseDouble();
	bool ParseBool();
	void ParseZeroTerminatedString(std::string& str);
	void ParseStringAfter32BitLength(std::string& str);
	Vec2 ParseVec2();
	Vec3 ParseVec3();
	Vec4 ParseVec4();
	IntVec2 ParseIntVec2();
	IntVec3 ParseIntVec3();
	IntVec4 ParseIntVec4();
	Rgba8 ParseRgba8();
	Rgba8 ParseRgb8();
	AABB2 ParseAABB2();
	AABB3 ParseAABB3();
	OBB2 ParseOBB2();
	OBB3 ParseOBB3();
	Plane2 ParsePlane2();
	Plane3 ParsePlane3();
	Vertex_PCU ParseVertexPCU();
	size_t GetCurReadPosition() const;
	void SetCurReadPosition(size_t readPosition);
	void SetBufferEndianMode(EndianMode mode);
	EndianMode GetBufferEndianMode() const;
	EndianMode GetLocalEndianMode() const;
protected:
	void SwitchByte(uint8_t* a, uint8_t* b) const;
	bool SafetyCheck(size_t steps) const;
protected:
	uint8_t const* m_buffer = nullptr;
	size_t m_size = 0;
	size_t m_currentPosition = 0;
	EndianMode m_localEndianMode;
	EndianMode m_bufferEndianMode = EndianMode::LITTLE;
};

inline void Reverse4BytesInPlace(void* ptrTo32BitDword)
{
	unsigned int* asUint32Ptr = reinterpret_cast<unsigned int*>(ptrTo32BitDword);
	unsigned int originalUint32 = *asUint32Ptr;
	unsigned int reversedUint32 = ((originalUint32 & 0x000000ff) << 24 |
		(originalUint32 & 0x0000ff00) << 8 |
		(originalUint32 & 0x00ff0000) >> 8 |
		(originalUint32 & 0xff000000) >> 24);
	*asUint32Ptr = reversedUint32;
}
inline void Reverse8BytesInPlace(void* ptrTo64BitQword)
{
	int64_t u = *(int64_t*)ptrTo64BitQword;

	*(int64_t*)ptrTo64BitQword = ((u & 0x00000000000000ff) << 56 |
		(u & 0x000000000000ff00) << 40 |
		(u & 0x0000000000ff0000) << 24 |
		(u & 0x00000000ff000000) << 8 |
		(u & 0x000000ff00000000) >> 8 |
		(u & 0x0000ff000000000) >> 24 |
		(u & 0x00ff00000000000) >> 40 |
		(u & 0xff0000000000000) >> 56);
}

inline void ReverseSizeTInPlace(void* ptrToSizeT)
{
	if constexpr (sizeof(ptrToSizeT) == 8)
	{
		Reverse8BytesInPlace(ptrToSizeT);
	}
	else
	{
		Reverse4BytesInPlace(ptrToSizeT);
	}
}