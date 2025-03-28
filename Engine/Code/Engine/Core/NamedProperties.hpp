#pragma once
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <typeinfo>
#include <string>  // For std::string
#include<map>

class TypedPropertiesBase
{
public:
	TypedPropertiesBase() = default;
	virtual ~TypedPropertiesBase() = default;
};

template <typename T> class TypedProperty : public TypedPropertiesBase
{
	friend class NamedProperties;
private:
	TypedProperty(T const& data)
		:m_data(data)
	{

	}
private:
	T m_data;
};
class NamedProperties
{
private:
	std::map<std::string, TypedPropertiesBase*> m_properties;
public:
	template <typename T> void SetValue(std::string const& keyName, T const& value);
	template <typename T> T GetValue(std::string const& keyName, T defaultValue) const;
	bool IsKeyNameValid(std::string const& keyName);
};

template <typename T>
void NamedProperties::SetValue(std::string const& keyName, T const& value)
{
	auto find = m_properties.find(keyName);

	if (find == m_properties.end())
	{
		m_properties[keyName] = new TypedProperty<T>(value);
	}
	else
	{
		TypedProperty<T>* asSameType = dynamic_cast<TypedProperty<T>*>(find->second);
		if (asSameType)
		{
			asSameType->m_data = value;
		}
		else
		{
			delete find->second;
			find->second = new TypedProperty<T>(value);
		}
	}
}

template <typename T>
T NamedProperties::GetValue(std::string const& keyName, T defaultValue) const
{
	auto find = m_properties.find(keyName);
	if (find == m_properties.end())
	{
		return defaultValue;
	}
	else
	{
		const std::type_info& a = typeid(*find->second);
		const char* raw_name = a.name();
		std::string as(raw_name);  // Convert to std::string

		const std::type_info& b = typeid(TypedProperty<T>);
		const char* r = b.name();
		std::string bs(r);  // Convert to std::string

		TypedProperty<T>* asTypedProperty = dynamic_cast<TypedProperty<T>*>(find->second);
		if (asTypedProperty)
		{
			return asTypedProperty->m_data;
		}
		else
		{
			ERROR_AND_DIE("NAMEPROPERTIES READING WRONG TYPE");
		}

	}
}


