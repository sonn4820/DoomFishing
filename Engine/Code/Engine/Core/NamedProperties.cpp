#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

bool NamedProperties::IsKeyNameValid(std::string const& keyName)
{
	auto found = m_properties.find(keyName);

	if (found == m_properties.end())
	{
		return false;
	}

	return true;
}
