#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <windows.h>

int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& fileName)
{
	FILE* filePointer;
	fopen_s(&filePointer, fileName.c_str(), "rb");

	if (!filePointer)
	{
		ERROR_AND_DIE("FILE IS NOT FOUND");
	}

	fseek(filePointer, 0, SEEK_END);
	long length = ftell(filePointer);
	fseek(filePointer, 0, SEEK_SET);

	outBuffer.resize(length);
	fread(outBuffer.data(), length, 1, filePointer);
	fclose(filePointer);

	return length;
}

int FileReadToString(std::string& outString, const std::string& fileName)
{
	std::vector<uint8_t> buffer;
	int result = FileReadToBuffer(buffer, fileName);
	buffer.push_back('\0');
	outString.assign(buffer.begin(), buffer.end());
	return result;
}

bool FileWriteFromBuffer(std::vector<uint8_t> const& buffer, std::string const& filePathName)
{
	FILE* file = nullptr;
	errno_t result = fopen_s(&file, filePathName.c_str(), "wb");
	if (result != 0 || file == nullptr)
	{
		return false;
	}
	fwrite(buffer.data(), 1, buffer.size(), file);
	fclose(file);
	return true;
}

bool CreateFolder(std::string const& folderPathName)
{
	return CreateDirectoryA(folderPathName.c_str(), nullptr);
}

bool HasFile(std::string const& folderPathName)
{
	struct stat sb;

	if (stat(folderPathName.c_str(), &sb) == 0 && !(sb.st_mode & S_IFDIR))
	{
		return true;
	}
	
	return false;
}

int WriteBufferToFile(std::vector<unsigned char>& outBuffer, std::string const& fileName)
{
	FILE* fileptr = nullptr;

	int error = fopen_s(&fileptr, fileName.c_str(), "wb");

	if (error == 0)
	{
		fwrite(outBuffer.data(), sizeof(unsigned char), outBuffer.size(), fileptr);
		fclose(fileptr);
	}

	return error;
}

bool LoadBinaryFileToExistingBuffer(std::string filePath, std::vector<unsigned char>& outBuffer)
{
	return FileReadToBuffer(outBuffer, filePath) == 0 ? false : true;
}

bool SaveBinaryFileFromBuffer(std::string filePath, std::vector<unsigned char> inBuffer)
{
	return WriteBufferToFile(inBuffer, filePath) == 0 ? true : false;
}

EndianMode GetPlatformLocalEndian()
{
	int n = 1;
	if (*(char*)&n == 1)
	{
		return EndianMode::LITTLE;
	}
	else {
		return EndianMode::BIG;
	}
}

BufferWriter::BufferWriter(std::vector<uint8_t>& bufferToWrite)
	:m_buffer(bufferToWrite)
{
	m_localEndianMode = GetPlatformLocalEndian();
}

void BufferWriter::AppendByte(uint8_t byteToAppend)
{
	m_buffer.push_back(byteToAppend);
}

void BufferWriter::AppendChar(char charToAppend)
{
	m_buffer.push_back(charToAppend);
}

void BufferWriter::AppendUshort(unsigned short ushortToAppend)
{
	uint8_t* charArray = (uint8_t*)&ushortToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[1]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
}

void BufferWriter::AppendShort(signed short shortToAppend)
{
	uint8_t* charArray = (uint8_t*)&shortToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[1]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
}

void BufferWriter::AppendUint32(unsigned int uintToAppend)
{
	uint8_t* charArray = (uint8_t*)&uintToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[3]);
		SwitchByte(&charArray[1], &charArray[2]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
}

void BufferWriter::AppendInt32(signed int intToAppend)
{
	uint8_t* charArray = (uint8_t*)&intToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[3]);
		SwitchByte(&charArray[1], &charArray[2]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
}

void BufferWriter::AppendUint64(uint64_t uintToAppend)
{
	uint8_t* charArray = (uint8_t*)&uintToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[7]);
		SwitchByte(&charArray[1], &charArray[6]);
		SwitchByte(&charArray[2], &charArray[5]);
		SwitchByte(&charArray[3], &charArray[4]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
	m_buffer.push_back(charArray[4]);
	m_buffer.push_back(charArray[5]);
	m_buffer.push_back(charArray[6]);
	m_buffer.push_back(charArray[7]);
}

void BufferWriter::AppendInt64(int64_t intToAppend)
{
	uint8_t* charArray = (uint8_t*)&intToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[7]);
		SwitchByte(&charArray[1], &charArray[6]);
		SwitchByte(&charArray[2], &charArray[5]);
		SwitchByte(&charArray[3], &charArray[4]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
	m_buffer.push_back(charArray[4]);
	m_buffer.push_back(charArray[5]);
	m_buffer.push_back(charArray[6]);
	m_buffer.push_back(charArray[7]);
}

void BufferWriter::AppendFloat(float floatToAppend)
{
	uint8_t* charArray = (uint8_t*)&floatToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[3]);
		SwitchByte(&charArray[1], &charArray[2]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
}

void BufferWriter::AppendDouble(double doubleToAppend)
{
	uint8_t* charArray = (uint8_t*)&doubleToAppend;

	if (m_localEndianMode != m_preferredEndianMode) 
	{
		SwitchByte(&charArray[0], &charArray[7]);
		SwitchByte(&charArray[1], &charArray[6]);
		SwitchByte(&charArray[2], &charArray[5]);
		SwitchByte(&charArray[3], &charArray[4]);
	}

	m_buffer.push_back(charArray[0]);
	m_buffer.push_back(charArray[1]);
	m_buffer.push_back(charArray[2]);
	m_buffer.push_back(charArray[3]);
	m_buffer.push_back(charArray[4]);
	m_buffer.push_back(charArray[5]);
	m_buffer.push_back(charArray[6]);
	m_buffer.push_back(charArray[7]);
}

void BufferWriter::AppendBool(bool boolToAppend)
{
	m_buffer.push_back(boolToAppend);
}

void BufferWriter::AppendZeroTerminatedString(std::string const& strToAppend)
{
	for (char i : strToAppend) 
	{
		AppendChar(i);
	}
	AppendChar('\0');
}

void BufferWriter::AppendStringAfter32BitLength(std::string const& strToAppend)
{
	AppendInt32((int)strToAppend.size());
	for (char i : strToAppend) 
	{
		AppendChar(i);
	}
}

void BufferWriter::AppendVec2(Vec2 const& vecToAppend)
{
	AppendFloat(vecToAppend.x);
	AppendFloat(vecToAppend.y);
}

void BufferWriter::AppendVec3(Vec3 const& vecToAppend)
{
	AppendFloat(vecToAppend.x);
	AppendFloat(vecToAppend.y);
	AppendFloat(vecToAppend.z);
}

void BufferWriter::AppendVec4(Vec4 const& vecToAppend)
{
	AppendFloat(vecToAppend.x);
	AppendFloat(vecToAppend.y);
	AppendFloat(vecToAppend.z);
	AppendFloat(vecToAppend.w);
}

void BufferWriter::AppendIntVec2(IntVec2 const& vecToAppend)
{
	AppendInt32(vecToAppend.x);
	AppendInt32(vecToAppend.y);
}

void BufferWriter::AppendIntVec3(IntVec3 const& vecToAppend)
{
	AppendInt32(vecToAppend.x);
	AppendInt32(vecToAppend.y);
	AppendInt32(vecToAppend.z);
}

void BufferWriter::AppendIntVec4(IntVec4 const& vecToAppend)
{
	AppendInt32(vecToAppend.x);
	AppendInt32(vecToAppend.y);
	AppendInt32(vecToAppend.z);
	AppendInt32(vecToAppend.w);
}

void BufferWriter::AppendRgba8(Rgba8 const colorToAppend)
{
	AppendByte(colorToAppend.r);
	AppendByte(colorToAppend.g);
	AppendByte(colorToAppend.b);
	AppendByte(colorToAppend.a);
}

void BufferWriter::AppendRgb8(Rgba8 const colorToAppend)
{
	AppendByte(colorToAppend.r);
	AppendByte(colorToAppend.g);
	AppendByte(colorToAppend.b);
}

void BufferWriter::AppendAABB2(AABB2 const& aabb2ToAppend)
{
	AppendVec2(aabb2ToAppend.m_mins);
	AppendVec2(aabb2ToAppend.m_maxs);
}

void BufferWriter::AppendAABB3(AABB3 const& aabb3ToAppend)
{
	AppendVec3(aabb3ToAppend.m_mins);
	AppendVec3(aabb3ToAppend.m_maxs);
}

void BufferWriter::AppendOBB2(OBB2 const& obb2ToAppend)
{
	AppendVec2(obb2ToAppend.m_center);
	AppendVec2(obb2ToAppend.m_halfDimensions);
	AppendVec2(obb2ToAppend.m_iBasisNormal);
}

void BufferWriter::AppendOBB3(OBB3 const& obb3ToAppend)
{
	AppendVec3(obb3ToAppend.m_center);
	AppendVec3(obb3ToAppend.m_halfDimensions);
	AppendVec3(obb3ToAppend.m_iBasisNormal);
	AppendVec3(obb3ToAppend.m_jBasisNormal);
}

void BufferWriter::AppendPlane2(Plane2 const& plane2ToAppend)
{
	AppendVec2(plane2ToAppend.m_normal);
	AppendFloat(plane2ToAppend.m_distanceFromOrigin);
}

void BufferWriter::AppendPlane3(Plane3 const& plane3ToAppend)
{
	AppendVec3(plane3ToAppend.m_normal);
	AppendFloat(plane3ToAppend.m_distanceFromOrigin);
}

void BufferWriter::AppendVertexPCU(Vertex_PCU const& vertToAppend)
{
	AppendVec3(vertToAppend.m_position);
	AppendRgba8(vertToAppend.m_color);
	AppendVec2(vertToAppend.m_uvTexCoords);
}

void BufferWriter::WriteIntToPos(size_t startPos, unsigned int intToWrite)
{
	if (startPos + sizeof(unsigned int) <= m_buffer.size()) {
		if (m_localEndianMode == m_preferredEndianMode) {
			m_buffer[startPos] = ((uint8_t*)&intToWrite)[0];
			m_buffer[startPos + 1] = ((uint8_t*)&intToWrite)[1];
			m_buffer[startPos + 2] = ((uint8_t*)&intToWrite)[2];
			m_buffer[startPos + 3] = ((uint8_t*)&intToWrite)[3];
		}
		else {
			m_buffer[startPos] = ((uint8_t*)&intToWrite)[3];
			m_buffer[startPos + 1] = ((uint8_t*)&intToWrite)[2];
			m_buffer[startPos + 2] = ((uint8_t*)&intToWrite)[1];
			m_buffer[startPos + 3] = ((uint8_t*)&intToWrite)[0];
		}
	}
}

void BufferWriter::SetPreferredEndianMode(EndianMode mode)
{
	if (mode != EndianMode::BIG && mode != EndianMode::LITTLE) {
		ERROR_AND_DIE("Cannot Set Endian Mode");
	}
	m_preferredEndianMode = mode;
}

EndianMode BufferWriter::GetPreferredEndianMode() const
{
	return m_preferredEndianMode;
}

EndianMode BufferWriter::GetLocalEndianMode() const
{
	return m_localEndianMode;
}

size_t BufferWriter::GetCurBufferSize() const
{
	return m_buffer.size();
}

void BufferWriter::SwitchByte(uint8_t* a, uint8_t* b) const
{
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}

BufferParser::BufferParser(std::vector<uint8_t> const& bufferToRead)
	:m_buffer(bufferToRead.data())
	, m_size(bufferToRead.size())
{
	int n = 1;
	if (*(char*)&n == 1) {
		m_localEndianMode = EndianMode::LITTLE;
	}
	else {
		m_localEndianMode = EndianMode::BIG;
	}
}

BufferParser::BufferParser(uint8_t const* const& buffer, size_t size)
	:m_buffer(buffer)
	, m_size(size)
{
	m_localEndianMode = GetPlatformLocalEndian();
}

uint8_t BufferParser::ParseByte()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(uint8_t)), Stringf("Cannot Parse a Byte in position %d, position out of buffer range", m_currentPosition));
	uint8_t value = m_buffer[m_currentPosition];
	m_currentPosition += sizeof(uint8_t);
	return value;
}

char BufferParser::ParseChar()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(char)), Stringf("Cannot Parse a Char in position %d, position out of buffer range", m_currentPosition));
	char value = m_buffer[m_currentPosition];
	m_currentPosition += sizeof(char);
	return value;
}

unsigned short BufferParser::ParseUshort()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(unsigned short)), Stringf("Cannot Parse a unsigned short in position %d, position out of buffer range", m_currentPosition));
	unsigned short value = *(unsigned short*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[1]));
	}

	m_currentPosition += sizeof(unsigned short);
	return value;
}

signed short BufferParser::ParseShort()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(signed short)), Stringf("Cannot Parse a signed short in position %d, position out of buffer range", m_currentPosition));
	signed short value = *(signed short*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[1]));
	}

	m_currentPosition += sizeof(signed short);
	return value;
}

unsigned int BufferParser::ParseUint32()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(unsigned int)), Stringf("Cannot Parse a unsigned int in position %d, position out of buffer range", m_currentPosition));
	unsigned int value = *(unsigned int*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[3]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[2]));
	}

	m_currentPosition += sizeof(unsigned int);
	return value;
}

signed int BufferParser::ParseInt32()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(signed int)), Stringf("Cannot Parse a signed int in position %d, position out of buffer range", m_currentPosition));
	signed int value = *(signed int*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[3]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[2]));
	}

	m_currentPosition += sizeof(signed int);
	return value;
}

uint64_t BufferParser::ParseUint64()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(uint64_t)), Stringf("Cannot Parse a unsigned int in position %d, position out of buffer range", m_currentPosition));
	uint64_t value = *(uint64_t*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[7]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[6]));
		SwitchByte(&(((uint8_t*)&value)[2]), &(((uint8_t*)&value)[5]));
		SwitchByte(&(((uint8_t*)&value)[3]), &(((uint8_t*)&value)[4]));
	}

	m_currentPosition += sizeof(uint64_t);
	return value;
}

int64_t BufferParser::ParseInt64()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(int64_t)), Stringf("Cannot Parse a signed int64 in position %d, position out of buffer range", m_currentPosition));
	int64_t value = *(int64_t*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[7]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[6]));
		SwitchByte(&(((uint8_t*)&value)[2]), &(((uint8_t*)&value)[5]));
		SwitchByte(&(((uint8_t*)&value)[3]), &(((uint8_t*)&value)[4]));
	}

	m_currentPosition += sizeof(int64_t);
	return value;
}

float BufferParser::ParseFloat()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(float)), Stringf("Cannot Parse a float in position %d, position out of buffer range", m_currentPosition));
	float value = *(float*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[3]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[2]));
	}

	m_currentPosition += sizeof(float);
	return value;
}

double BufferParser::ParseDouble()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(double)), Stringf("Cannot Parse a double in position %d, position out of buffer range", m_currentPosition));
	double value = *(double*)(&m_buffer[m_currentPosition]);

	if (m_localEndianMode != m_bufferEndianMode) {
		SwitchByte(&(((uint8_t*)&value)[0]), &(((uint8_t*)&value)[7]));
		SwitchByte(&(((uint8_t*)&value)[1]), &(((uint8_t*)&value)[6]));
		SwitchByte(&(((uint8_t*)&value)[2]), &(((uint8_t*)&value)[5]));
		SwitchByte(&(((uint8_t*)&value)[3]), &(((uint8_t*)&value)[4]));
	}

	m_currentPosition += sizeof(double);
	return value;
}

bool BufferParser::ParseBool()
{
	GUARANTEE_OR_DIE(SafetyCheck(sizeof(bool)), Stringf("Cannot Parse a bool in position %d, position out of buffer range", m_currentPosition));

	bool value = *(bool*)(&m_buffer[m_currentPosition]);
	m_currentPosition += sizeof(bool);
	return value;
}

void BufferParser::ParseZeroTerminatedString(std::string& out_str)
{
	char* str = (char*)&m_buffer[m_currentPosition];
	// ToDo: a safer version needed! There may be infinite loop
	while (m_buffer[m_currentPosition++] != '\0');
	out_str = std::string(str);
}

void BufferParser::ParseStringAfter32BitLength(std::string& stringValue)
{
	uint32_t stringLength = ParseUint32();

	for (uint32_t i = 0; i < stringLength; i++)
	{
		char character = ParseChar();
		stringValue.push_back(character);
	}
}

Vec2 BufferParser::ParseVec2()
{
	Vec2 value;
	value.x = ParseFloat();
	value.y = ParseFloat();
	return value;
}

Vec3 BufferParser::ParseVec3()
{
	Vec3 value;
	value.x = ParseFloat();
	value.y = ParseFloat();
	value.z = ParseFloat();
	return value;
}

Vec4 BufferParser::ParseVec4()
{
	Vec4 value;
	value.x = ParseFloat();
	value.y = ParseFloat();
	value.z = ParseFloat();
	value.w = ParseFloat();
	return value;
}

IntVec2 BufferParser::ParseIntVec2()
{
	IntVec2 value;
	value.x = ParseInt32();
	value.y = ParseInt32();
	return value;
}

IntVec3 BufferParser::ParseIntVec3()
{
	IntVec3 value;
	value.x = ParseInt32();
	value.y = ParseInt32();
	value.z = ParseInt32();
	return value;
}

IntVec4 BufferParser::ParseIntVec4()
{
	IntVec4 value;
	value.x = ParseInt32();
	value.y = ParseInt32();
	value.z = ParseInt32();
	value.w = ParseInt32();
	return value;
}

Rgba8 BufferParser::ParseRgba8()
{
	Rgba8 value;
	value.r = ParseByte();
	value.g = ParseByte();
	value.b = ParseByte();
	value.a = ParseByte();
	return value;
}

Rgba8 BufferParser::ParseRgb8()
{
	Rgba8 value;
	value.r = ParseByte();
	value.g = ParseByte();
	value.b = ParseByte();
	value.a = 255;
	return value;
}

AABB2 BufferParser::ParseAABB2()
{
	AABB2 value;
	value.m_mins = ParseVec2();
	value.m_maxs = ParseVec2();
	return value;
}

AABB3 BufferParser::ParseAABB3()
{
	AABB3 value;
	value.m_mins = ParseVec3();
	value.m_maxs = ParseVec3();
	return value;
}

OBB2 BufferParser::ParseOBB2()
{
	OBB2 value;
	value.m_center = ParseVec2();
	value.m_halfDimensions = ParseVec2();
	value.m_iBasisNormal = ParseVec2();
	return value;
}

OBB3 BufferParser::ParseOBB3()
{
	OBB3 value;
	value.m_center = ParseVec3();
	value.m_halfDimensions = ParseVec3();
	value.m_iBasisNormal = ParseVec3();
	value.m_jBasisNormal = ParseVec3();
	return value;
}

Plane2 BufferParser::ParsePlane2()
{
	Plane2 value;
	value.m_normal = ParseVec2();
	value.m_distanceFromOrigin = ParseFloat();
	return value;
}

Plane3 BufferParser::ParsePlane3()
{
	Plane3 value;
	value.m_normal = ParseVec3();
	value.m_distanceFromOrigin = ParseFloat();
	return value;
}

Vertex_PCU BufferParser::ParseVertexPCU()
{
	Vertex_PCU vert;
	vert.m_position = ParseVec3();
	vert.m_color = ParseRgba8();
	vert.m_uvTexCoords = ParseVec2();
	return vert;
}

size_t BufferParser::GetCurReadPosition() const
{
	return m_currentPosition;
}

void BufferParser::SetCurReadPosition(size_t readPosition)
{
	m_currentPosition = readPosition;
}

void BufferParser::SetBufferEndianMode(EndianMode mode)
{
	if (mode != EndianMode::BIG && mode != EndianMode::LITTLE) 
	{
		ERROR_AND_DIE("Cannot Set Endian Mode");
	}
	m_bufferEndianMode = mode;
}

EndianMode BufferParser::GetBufferEndianMode() const
{
	return m_bufferEndianMode;
}

EndianMode BufferParser::GetLocalEndianMode() const
{
	return m_localEndianMode;
}

void BufferParser::SwitchByte(uint8_t* a, uint8_t* b) const
{
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}

bool BufferParser::SafetyCheck(size_t steps) const
{
	if (m_currentPosition + steps > m_size) {
		return false;
	}
	return true;
}
