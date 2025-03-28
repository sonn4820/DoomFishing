#include "HashedCaseInsensitiveString.hpp"

HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& text)
	:HashedCaseInsensitiveString(text.c_str())
{

}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copy)
	:m_originalString(copy.m_originalString), m_caseInsensitiveHash(copy.m_caseInsensitiveHash)
{

}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(char const* text)
	:m_originalString(text), m_caseInsensitiveHash(GenerateCaseInsensitiveHash(text))
{

}

bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& rhs) const
{
	if (m_caseInsensitiveHash != rhs.m_caseInsensitiveHash) return false;
	return 0 == _stricmp(m_originalString.c_str(), rhs.m_originalString.c_str());
}

bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& rhs) const
{
	if (m_caseInsensitiveHash < rhs.m_caseInsensitiveHash) return true;
	if (m_caseInsensitiveHash > rhs.m_caseInsensitiveHash) return false;
	return _stricmp(m_originalString.c_str(), rhs.m_originalString.c_str()) < 0;
}

bool HashedCaseInsensitiveString::operator==(std::string const& text) const
{
	HashedCaseInsensitiveString compare = HashedCaseInsensitiveString(text);
	return *this == compare;
}

bool HashedCaseInsensitiveString::operator==(const char* text) const
{
	HashedCaseInsensitiveString compare = HashedCaseInsensitiveString(text);
	return *this == compare;
}

bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& rhs) const
{
	if (m_caseInsensitiveHash != rhs.m_caseInsensitiveHash) return true;
	return 0 != _stricmp(m_originalString.c_str(), rhs.m_originalString.c_str());
}

bool HashedCaseInsensitiveString::operator!=(const char* text) const
{
	HashedCaseInsensitiveString compare = HashedCaseInsensitiveString(text);
	return *this != compare;
}

bool HashedCaseInsensitiveString::operator!=(std::string const& text) const
{
	HashedCaseInsensitiveString compare = HashedCaseInsensitiveString(text);
	return *this != compare;
}

void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& assignFrom)
{
	m_originalString = assignFrom.m_originalString;
	m_caseInsensitiveHash = assignFrom.m_caseInsensitiveHash;
}

void HashedCaseInsensitiveString::operator=(const char* text)
{
	*this = HashedCaseInsensitiveString(text);
}

void HashedCaseInsensitiveString::operator=(std::string& text)
{
	*this = HashedCaseInsensitiveString(text);
}

unsigned int HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(std::string const& text)
{
	return GenerateCaseInsensitiveHash(text.c_str());
}

unsigned int HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(char const* text)
{
	unsigned int hash = 0;
	char const* scan = text;
	while (*scan != '\0')
	{
		hash *= 31;
		hash += tolower(*scan);
		++scan;
	}

	return hash;
}
