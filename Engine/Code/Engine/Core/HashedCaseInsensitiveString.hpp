#pragma once
#include "Engine/Core/StringUtils.hpp"
class HashedCaseInsensitiveString
{
public:
	HashedCaseInsensitiveString() = default;
	HashedCaseInsensitiveString(std::string const& text);
	HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copy);
	HashedCaseInsensitiveString(char const* text);

	unsigned int GetHash() const { return m_caseInsensitiveHash; }
	std::string const& GetOriginalString() const { return m_originalString; }
	char const* c_str() const { return m_originalString.c_str(); }

	bool operator==(HashedCaseInsensitiveString const& rhs) const;
	bool operator<(HashedCaseInsensitiveString const& rhs)const;
	bool operator!=(HashedCaseInsensitiveString const& rhs)const;

	bool operator==(const char* text) const;
	bool operator!=(const char* text) const;

	bool operator==(std::string const& text) const;
	bool operator!=(std::string const& text) const;

	void operator=(HashedCaseInsensitiveString const& assignFrom);
	void operator=(const char* text);
	void operator=(std::string& text);


	static unsigned int GenerateCaseInsensitiveHash(std::string const& text);
	static unsigned int GenerateCaseInsensitiveHash(char const* text);

private:
	std::string m_originalString;
	unsigned int m_caseInsensitiveHash = 0;
};

