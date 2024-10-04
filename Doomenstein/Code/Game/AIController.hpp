#pragma once
#include "Game/Controller.hpp"

class AIController : public Controller
{
public:
	virtual ~AIController();
	virtual bool IsPlayer() const;
	virtual void Possess(ActorUID actorUID) override;
	virtual void Update();
	virtual void DamagedBy(ActorUID damageDealer) override;

public:
	ActorUID m_targetUID = ActorUID::INVALID;
	bool m_isAggressive = false;
private:
	ActorUID FindClosestEnemy();
	void ActionWhenFoundEnemy(ActorUID enemy);
private:
	float m_movementSpeed = 0.f;
};