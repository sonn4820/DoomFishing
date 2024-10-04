#include "Game/Controller.hpp"
#include "Game/PlayerController.hpp"
#include "Game/FishAIController.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"

FishAIController::~FishAIController()
{

}

bool FishAIController::IsPlayer() const
{
	return false;
}

void FishAIController::Possess(ActorUID actorUID)
{
	Controller::Possess(actorUID);
	float randomTimer = g_theRNG->RollRandomFloatInRange(3.f, 6.f);
	m_detectTimer = new Timer(randomTimer, g_theGame->m_clock);
	float randomTurnTimer = g_theRNG->RollRandomFloatInRange(3.f, 10.f);
	m_randomMovementTimer = new Timer(randomTurnTimer, g_theGame->m_clock);
	m_randomMovementTimer->Start();
	RandomGenerateHeatMap();
}

void FishAIController::Update()
{
	if (m_controllerActorUID->m_isDead)
	{
		return;
	}
	switch (m_currentState)
	{
	case PATROLLING:
		Patrolling();
		break;
	case DETECTING:
		Detecting();
		break;
	case BITING:
		Biting();
		break;
	}
}

void FishAIController::DamagedBy(ActorUID damageDealer)
{
	UNUSED(damageDealer);
}

ActorUID FishAIController::FindClosestBait()
{
	if (g_currentMap->GetActorByUID(m_targetBaitUID))
	{
		return m_targetBaitUID;
	}

	ActorUID closestActors = g_currentMap->GetClosestVisibleEnemy(m_controllerActorUID, "Bait");
	if (closestActors.IsValid() && !closestActors->m_hasFishAttached)
	{
		if (!g_currentMap->IsTileInBoundFromPos(closestActors->m_position) || !g_currentMap->GetTileFromPos(closestActors->m_position)->m_tileDef->m_isWater)
		{
			return ActorUID::INVALID;
		}
		Vec3 toClosestActors = closestActors->m_position - m_controllerActorUID->m_position;

		if (DotProduct3D(m_controllerActorUID->GetActorForward(), toClosestActors) <= 0.f)
		{
			return ActorUID::INVALID;
		}
		else
		{
			float distanceBetween = (closestActors->m_position - m_controllerActorUID->m_position).GetLength();
			float angleBetween = GetAngleDegreesBetweenVectorsXY3D(m_controllerActorUID->GetActorForward(), (closestActors->m_position - m_controllerActorUID->m_position).GetNormalized());
			float maxDist = m_controllerActorUID->m_actorDef->m_AISightRadius;
			float maxAngle = m_controllerActorUID->m_actorDef->m_AIAngle * 0.5f;
			if (distanceBetween < maxDist && angleBetween < maxAngle)
			{
				Vec3 toTargetNormalized = (closestActors->m_position - m_controllerActorUID->m_position).GetNormalized();
				RaycastResult rayResult = g_currentMap->RaycastAll(m_controllerActorUID->m_position, toTargetNormalized, maxDist, m_controllerActorUID->m_UID);
				if (rayResult.m_didImpact && rayResult.m_targetUID == closestActors)
				{
					closestActors->m_hasFishAttached = true;
					m_playerWhoCaught = closestActors->m_owner;
					return closestActors;
				}
			}
		}

	}
	return ActorUID::INVALID;
}

void FishAIController::Patrolling()
{
	m_targetBaitUID = FindClosestBait();
	if (m_detectTimer->IsStopped())
	{
		m_detectTimer->Start();
		m_controllerActorUID->m_isStatic = false;
		m_controllerActorUID->m_isFlying = false;
	}
	if (m_randomMovementTimer->DecrementPeriodIfElapsed())
	{
		m_randomSpeed = g_theRNG->RollRandomFloatInRange(m_controllerActorUID->m_actorDef->m_walkSpeed, m_controllerActorUID->m_actorDef->m_runSpeed);
		m_randomOrientation = g_theRNG->RollRandomFloatInRange(0.f, 360.f);
	}
	if (m_targetBaitUID.IsValid())
	{
		if (m_detectTimer->HasPeriodElapsed())
		{
			if (g_theRNG->RollRandomChance(m_controllerActorUID->m_actorDef->m_detectionRate))
			{
				m_detectTimer->Stop();
				SwitchState(DETECTING);
				return;
			}
			else
			{
				m_detectTimer->Start();
			}

		}
	}
	if (m_controllerActorUID->m_actorDef->m_useHeatmap)
	{
		IntVec2 currentCoord = IntVec2(RoundDownToInt(m_controllerActorUID->m_position.x), RoundDownToInt(m_controllerActorUID->m_position.y));
		if (currentCoord == m_goalCoord)
		{
			RandomGenerateHeatMap();
		}
		MoveToLowestHeatNeighborsTile();
		m_controllerActorUID->AddForce(m_controllerActorUID->GetActorForward(), m_controllerActorUID->m_actorDef->m_walkSpeed);
	}
	else
	{
		m_controllerActorUID->TurnInDirection(m_randomOrientation, m_controllerActorUID->m_actorDef->m_turnSpeed * g_theGame->m_clock->GetDeltaSeconds());
		m_controllerActorUID->AddForce(m_controllerActorUID->GetActorForward(), m_randomSpeed);
	}

}

void FishAIController::Detecting()
{
	if (m_targetBaitUID.IsValid() && g_currentMap->GetActorByUID(m_targetBaitUID))
	{
		if (m_targetBaitUID->m_isDead || m_targetBaitUID->m_position.z > 0.2f)
		{
			m_targetBaitUID->m_hasFishAttached = false;
			m_targetBaitUID = ActorUID::INVALID;
			SwitchState(PATROLLING);
			return;
		}

		float distanceBetween = (m_targetBaitUID->m_position - m_controllerActorUID->m_position).GetLength();
		Vec3 toTargetNormalized = (m_targetBaitUID->m_position - m_controllerActorUID->m_position).GetNormalized();
		float goalAngle = toTargetNormalized.GetAngleAboutZDegrees();
		m_controllerActorUID->TurnInDirection(goalAngle, m_controllerActorUID->m_actorDef->m_turnSpeed * g_theGame->m_clock->GetDeltaSeconds());

		float stopDistance = m_targetBaitUID->m_actorDef->m_physiscRadius + m_controllerActorUID->m_actorDef->m_physiscRadius + m_controllerActorUID->m_actorDef->m_physiscRadius * 0.1f + m_targetBaitUID->m_actorDef->m_physiscRadius * 0.1f;
		if (distanceBetween > stopDistance)
		{
			m_controllerActorUID->MoveInDirection((m_targetBaitUID->m_position - m_controllerActorUID->m_position).GetNormalized(), m_controllerActorUID->m_actorDef->m_walkSpeed);
		}
		else
		{
			SwitchState(BITING);
		}
	}
	else
	{
		SwitchState(PATROLLING);
	}
}

void FishAIController::Biting()
{
	if (!g_currentMap->GetActorByUID(m_targetBaitUID))
	{
		if (g_currentMap->IsPositionInBound(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y, 0.f) 
			&& g_currentMap->GetTileFromPos(m_controllerActorUID->m_position)->m_tileDef->m_isWater)
		{
			m_targetBaitUID = ActorUID::INVALID;
			SwitchState(PATROLLING);
			return;
		}
		if (m_controllerActorUID->m_isDead)
		{
			return;
		}
		m_recordPos.z = 0.2f;
		m_controllerActorUID->m_position = m_recordPos;
		reinterpret_cast<PlayerController*>(m_playerWhoCaught->m_controller)->AddMoney(m_controllerActorUID->m_actorDef->m_killMoney);
		m_controllerActorUID->Die();
	}
	else
	{
		m_controllerActorUID->m_isStatic = true;
		m_controllerActorUID->m_isFlying = true;
		if (!m_targetBaitUID->m_isDead)
		{
			m_controllerActorUID->m_position = m_targetBaitUID->m_position - m_controllerActorUID->GetActorForward() * m_controllerActorUID->m_actorDef->m_physiscRadius;
			m_recordPos = m_targetBaitUID->m_position;
			if (m_controllerActorUID->m_position.z > 0.2f)
			{
				m_controllerActorUID->PlaySoundByName("Pull", false, 5.f);
			}
		}
		else
		{
			if (g_currentMap->IsPositionInBound(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y, 0.f)
				&& g_currentMap->GetTileFromPos(m_controllerActorUID->m_position)->m_tileDef->m_isWater)
			{
				m_targetBaitUID = ActorUID::INVALID;
				SwitchState(PATROLLING);
				return;
			}
			else
			{
				m_targetBaitUID->Die();
			}
		}
	}
}

void FishAIController::RandomGenerateHeatMap()
{
	m_heatMap = new TileHeatMap(g_currentMap->m_dimensions);
	for (;;)
	{
		m_goalCoord = IntVec2(g_theRNG->RollRandomIntInRange(1, g_currentMap->m_dimensions.x - 2), g_theRNG->RollRandomIntInRange(1, g_currentMap->m_dimensions.y - 2));
		if (!g_currentMap->IsTileWater(m_goalCoord))
		{
			continue;
		}
		g_currentMap->PopulateFishDistanceField(*m_heatMap, m_goalCoord, 9999.f);
		break;
	}
}

void FishAIController::MoveToLowestHeatNeighborsTile()
{
	IntVec2 currertCoord = IntVec2(RoundDownToInt(m_controllerActorUID->m_position.x), RoundDownToInt(m_controllerActorUID->m_position.y));
	IntVec2 neighborCoords[4];
	neighborCoords[0] = currertCoord + IntVec2::EAST;
	neighborCoords[1] = currertCoord + IntVec2::WEST;
	neighborCoords[2] = currertCoord + IntVec2::NORTH;
	neighborCoords[3] = currertCoord + IntVec2::SOUTH;

	float neighborHeat[4];
	neighborHeat[0] = m_heatMap->GetHeatAt(neighborCoords[0]);
	neighborHeat[1] = m_heatMap->GetHeatAt(neighborCoords[1]);
	neighborHeat[2] = m_heatMap->GetHeatAt(neighborCoords[2]);
	neighborHeat[3] = m_heatMap->GetHeatAt(neighborCoords[3]);

	float lowestHeat = m_heatMap->GetHeatAt(currertCoord);
	IntVec2 lowestHeatCoords;
	for (int i = 0; i < 4; i++)
	{
		if ((!g_currentMap->IsTileWater(neighborCoords[i]) && !g_currentMap->IsTileBridge(neighborCoords[i])) || m_heatMap->GetHeatAt(neighborCoords[i]) == -1.f
			|| (g_currentMap->GetActorByUID(m_targetBaitUID) && 
				g_currentMap->GetTileFromPos(m_targetBaitUID->m_position)->m_tileCoords == neighborCoords[i]))
		{
	
			neighborHeat[i] = 9999.f;
		}
		if (neighborHeat[i] <= lowestHeat)
		{
			lowestHeatCoords = neighborCoords[i];
		}
	}
	Vec2 currentWayPoint = Vec2(lowestHeatCoords.x + 0.5f, lowestHeatCoords.y + 0.5f);

// 	DebugAddWorldLine(m_controllerActorUID->m_position, Vec3(currentWayPoint.x, currentWayPoint.y, 0.5f), 0.025f, 0, Rgba8::COLOR_DARK_RED);
// 	DebugAddWorldLine(m_controllerActorUID->m_position, Vec3(m_goalCoord.x, m_goalCoord.y, 0.5f), 0.025f, 0, Rgba8::COLOR_GREEN);
	Vec2 dirToWayPointCenter = (currentWayPoint - Vec2(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y)).GetNormalized();
	m_controllerActorUID->TurnInDirection(dirToWayPointCenter.GetOrientationDegrees(), m_controllerActorUID->m_actorDef->m_turnSpeed * g_theGame->m_clock->GetDeltaSeconds());
}

FishState FishAIController::SwitchState(FishState targetState)
{
	if (targetState == BITING)
	{
		m_controllerActorUID->PlaySoundByName("Bite", true, 3.f);
	}
	m_currentState = targetState;
	return m_currentState;
}
