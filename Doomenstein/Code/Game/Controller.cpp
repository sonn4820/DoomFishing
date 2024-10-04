#include "Game/Controller.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"

void Controller::Possess(ActorUID actorUID)
{
	if (actorUID.IsValid() && g_currentMap->GetActorByUID(actorUID))
	{
		if (m_controllerActorUID.IsValid() && m_controllerActorUID.GetActor() != nullptr)
		{
			m_controllerActorUID->OnUnpossessed();
		}

		m_controllerActorUID = actorUID;
		m_controllerActorUID->OnPossessed(this);
		m_controllerActorUID->PlayAnimationByName("Default");
	}
}

Actor* Controller::GetActor() const
{
	if (m_controllerActorUID.IsValid())
	{
		return m_controllerActorUID.GetActor();
	}

	return nullptr;
}