#pragma once
#include "Game/Controller.hpp"
#include "Engine/Core/HeatMaps.hpp"

enum FishState
{
	PATROLLING,
	DETECTING,
	BITING,
};
class FishAIController : public Controller
{
public:
	virtual ~FishAIController();
	virtual bool IsPlayer() const;
	virtual void Possess(ActorUID actorUID) override;
	virtual void Update();
	virtual void DamagedBy(ActorUID damageDealer) override;

public:
	ActorUID m_targetBaitUID = ActorUID::INVALID;
	FishState m_currentState = PATROLLING;
	float m_randomSpeed = 1.f;
	float m_randomOrientation = 0.f;
private:
	ActorUID FindClosestBait();

	void Patrolling();
	void Detecting();
	void Biting();

	void RandomGenerateHeatMap();
	void MoveToLowestHeatNeighborsTile();

	FishState SwitchState(FishState targetState);
private:
	Vec3 m_recordPos;
	Timer* m_detectTimer = nullptr;
	Timer* m_randomMovementTimer = nullptr;
	TileHeatMap* m_heatMap = nullptr;
	IntVec2 m_goalCoord;
	ActorUID m_playerWhoCaught = ActorUID::INVALID;

};