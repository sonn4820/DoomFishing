#pragma once
#include "Game/Actor.hpp"

class Controller
{
public:
	virtual ~Controller() = default;
	virtual bool IsPlayer() const = 0;
	virtual void Update() = 0;
	virtual void Possess(ActorUID actorUID);
	virtual Actor* GetActor() const;
	virtual void DamagedBy(ActorUID damageDealer) = 0;

public:
	ActorUID m_controllerActorUID = ActorUID::INVALID;
};