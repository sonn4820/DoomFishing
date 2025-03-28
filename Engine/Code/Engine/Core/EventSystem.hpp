#pragma once
#include "Engine/Window/Window.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/HashedCaseInsensitiveString.hpp"
#include <string>
#include <vector>
#include <mutex>


typedef NamedProperties EventArgs;
typedef bool (*EventCallbackFunctionPtrType)(EventArgs&);

struct EventSubscriptionBase
{
	EventSubscriptionBase() = default;
	virtual ~EventSubscriptionBase() = default;
	virtual bool Fire(EventArgs& args) = 0;
	virtual bool IsForObject([[maybe_unused]]void* objectPtr) { return false; };
};
struct EventSubscriptionFunction : public EventSubscriptionBase
{
	EventSubscriptionFunction(EventCallbackFunctionPtrType functionPtr)
		:m_functionPtr(functionPtr)
	{

	}
	EventCallbackFunctionPtrType m_functionPtr = nullptr;

	bool Fire(EventArgs& args) override
	{
		return m_functionPtr(args);
	}
};

template<typename T_ObjectType>
struct EventSubscriptionMethod : public EventSubscriptionBase
{
	typedef bool (T_ObjectType::*EventCallbackMemberFunctionPtrType)(EventArgs& arg);

	EventSubscriptionMethod(T_ObjectType* pointerToObject, EventCallbackMemberFunctionPtrType methodPtr)
		:m_object(pointerToObject), m_methodPtr(methodPtr)
	{

	}
	EventCallbackMemberFunctionPtrType	m_methodPtr = nullptr;
	T_ObjectType*						m_object = nullptr;

	bool Fire(EventArgs& args) override
	{
		return (m_object->*m_methodPtr)(args);
	}
	bool IsForObject(void* objectPtr) override
	{
		return objectPtr == (void*) m_object;
	}
	bool IsForMethod(bool (T_ObjectType::* methodPtr)(EventArgs& arg))
	{
		return m_methodPtr == methodPtr;
	};
};

typedef std::vector<EventSubscriptionBase*> SubscriptionList;

struct EventSystemConfig
{
	Window* m_window = nullptr;
};


class EventSystem
{
public:
	EventSystem(EventSystemConfig const& config);
	~EventSystem();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr);

	template<typename T_ObjectType>
	void SubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr, bool(T_ObjectType::*methodPtr)(EventArgs& arg));
	template<typename T_ObjectType>
	void UnsubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr, bool(T_ObjectType::*methodPtr)(EventArgs& arg));
	

	void FireEvent(std::string const& eventName, EventArgs& arg);
	void FireEvent(std::string const& eventName);
	Strings GetAllRegisteredEvent();

protected:
	EventSystemConfig m_config;
	std::map<HashedCaseInsensitiveString, SubscriptionList> m_subscriptionListByEventName;

};


template<typename T_ObjectType>
void EventSystem::SubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr, bool(T_ObjectType::* methodPtr)(EventArgs& arg))
{
	HashedCaseInsensitiveString HCIS = HashedCaseInsensitiveString(eventName);
	SubscriptionList& subcribeListObject = m_subscriptionListByEventName[HCIS];
	EventSubscriptionMethod<T_ObjectType>* newSubscriber = new EventSubscriptionMethod<T_ObjectType>(objectPtr, methodPtr);
	subcribeListObject.push_back(newSubscriber);
}

template<typename T_ObjectType>
void EventSystem::UnsubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr, bool(T_ObjectType::* methodPtr)(EventArgs& arg))
{
	HashedCaseInsensitiveString HCIS = HashedCaseInsensitiveString(eventName);
	auto found = m_subscriptionListByEventName.find(HCIS);
	if (found != m_subscriptionListByEventName.end())
	{
		SubscriptionList& subscribersForThisEvent = found->second;
		for (int i = 0; i < (int)subscribersForThisEvent.size(); i++)
		{
			if (subscribersForThisEvent[i]->IsForObject(objectPtr))
			{
				EventSubscriptionMethod<T_ObjectType>* subscriber = dynamic_cast<EventSubscriptionMethod<T_ObjectType>*>(subscribersForThisEvent[i]);

				if (subscriber->IsForMethod(methodPtr))
				{
					delete subscriber;
					subscribersForThisEvent.erase(subscribersForThisEvent.begin() + i);
					--i;
				}
			}
		}
	}
}

extern EventSystem* g_theEventSystem;
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr);
void FireEvent(std::string const& eventName, EventArgs& arg);
void FireEvent(std::string const& eventName);