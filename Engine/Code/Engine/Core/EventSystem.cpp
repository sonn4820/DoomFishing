#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/HashedCaseInsensitiveString.hpp"

EventSystem* g_theEventSystem = nullptr;

EventSystem::EventSystem(EventSystemConfig const& config)
	:m_config(config)
{

}

EventSystem::~EventSystem()
{
}

void EventSystem::Startup()
{
}

void EventSystem::BeginFrame()
{
}

void EventSystem::EndFrame()
{
}

void EventSystem::Shutdown()
{
}

void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr)
{
	HashedCaseInsensitiveString HCIS = HashedCaseInsensitiveString(eventName);
	SubscriptionList& subcribeListObject = m_subscriptionListByEventName[HCIS];
	EventSubscriptionFunction* newSubscriber = new EventSubscriptionFunction(functionPtr);
	subcribeListObject.push_back(newSubscriber);
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr)
{
	HashedCaseInsensitiveString HCIS = HashedCaseInsensitiveString(eventName);
	auto found = m_subscriptionListByEventName.find(HCIS);
	if (found != m_subscriptionListByEventName.end())
	{
		SubscriptionList& subscribersForThisEvent = found->second;
		for (int i = 0; i < (int)subscribersForThisEvent.size(); i++)
		{
			EventSubscriptionFunction* subscriber = dynamic_cast<EventSubscriptionFunction*>(subscribersForThisEvent[i]);
			if (subscriber->m_functionPtr == functionPtr)
			{
				delete subscriber;
				subscribersForThisEvent.erase(subscribersForThisEvent.begin() + i);
				--i;
			}
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName, EventArgs& arg)
{
	HashedCaseInsensitiveString eventToFire = HashedCaseInsensitiveString(eventName);
	HashedCaseInsensitiveString existedEvent;

	for (const std::pair<HashedCaseInsensitiveString, SubscriptionList>& pair : m_subscriptionListByEventName)
	{
		if (pair.first == eventToFire)
		{
			existedEvent = pair.first;
			break;
		}
	}
	auto found = m_subscriptionListByEventName.find(existedEvent);
	if (found != m_subscriptionListByEventName.end())
	{
		SubscriptionList subscribersForThisEvent = found->second;
		for (EventSubscriptionBase* subscriber : subscribersForThisEvent)
		{
			bool wasConsumed = subscriber->Fire(arg);
			if (wasConsumed)
			{
				break;
			}
		}
	}
	else
	{
		if (g_theDevConsole)
		{
			g_theDevConsole->AddLine(DevConsole::ERROR, "ERROR: Unknown command: " + eventToFire.GetOriginalString());
		}
	}
}

void EventSystem::FireEvent(std::string const& eventName)
{
	EventArgs emptyArg;
	FireEvent(ToLower(eventName), emptyArg);
}

Strings EventSystem::GetAllRegisteredEvent()
{
	Strings allResgisteredEvent;
	for (const std::pair<HashedCaseInsensitiveString, SubscriptionList>& pair : m_subscriptionListByEventName)
	{
		const std::string& eventName = pair.first.GetOriginalString();
		allResgisteredEvent.push_back(eventName);
	}
	return allResgisteredEvent;
}

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr)
{
	g_theEventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunctionPtrType functionPtr)
{
	g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
}

void FireEvent(std::string const& eventName, EventArgs& arg)
{
	g_theEventSystem->FireEvent(eventName, arg);
}

void FireEvent(std::string const& eventName)
{
	g_theEventSystem->FireEvent(eventName);
}
