#include "Game/Controller.hpp"
#include "Game/AIController.hpp"
#include "Game/PlayerController.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"

AIController::~AIController()
{

}

bool AIController::IsPlayer() const
{
	return false;
}

void AIController::Possess(ActorUID actorUID)
{
	Controller::Possess(actorUID);
	m_movementSpeed = m_controllerActorUID->m_actorDef->m_runSpeed;
}

void AIController::Update()
{
	if (m_controllerActorUID->m_isDead)
	{
		return;
	}
	ActionWhenFoundEnemy(FindClosestEnemy());
}

void AIController::DamagedBy(ActorUID damageDealer)
{
	m_targetUID = damageDealer;
	m_isAggressive = true;
	if (!m_controllerActorUID->m_isDead)
	{
		m_controllerActorUID->PlayAnimationByName("Hurt");
		m_controllerActorUID->PlaySoundByName("Hurt", false);
	}
}

ActorUID AIController::FindClosestEnemy()
{
	if (m_targetUID.IsValid())
	{
		return m_targetUID;
	}

	ActorUID closestActors = g_currentMap->GetClosestVisibleEnemy(m_controllerActorUID, "Marine");
	if (closestActors.IsValid())
	{
		if (m_isAggressive)
		{
			return closestActors;
		}

		Vec3 toClosestActors = closestActors->m_position - m_controllerActorUID->m_position;

		if (DotProduct3D(m_controllerActorUID->GetActorForward(), toClosestActors) <= 0.f)
		{
			m_isAggressive = false;
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
					return closestActors;
				}
			}
		}

	}
	return ActorUID::INVALID;
}

void AIController::ActionWhenFoundEnemy(ActorUID enemy)
{
	m_targetUID = enemy;

	if (m_targetUID.IsValid() && g_currentMap->GetActorByUID(m_targetUID))
	{
		if (m_targetUID->m_isDead)
		{
			m_targetUID = ActorUID::INVALID;
			if (m_isAggressive)
			{
				m_isAggressive = false;
			}
			return;
		}

 		float distanceBetween = (m_targetUID->m_position - m_controllerActorUID->m_position).GetLength();
		Vec3 toTargetNormalized = (m_targetUID->m_position - m_controllerActorUID->m_position).GetNormalized();
		float goalAngle = toTargetNormalized.GetAngleAboutZDegrees();
		float turnSpeed = m_controllerActorUID->m_actorDef->m_turnSpeed;
		m_controllerActorUID->TurnInDirection(goalAngle, turnSpeed * g_theGame->m_clock->GetDeltaSeconds());

		float stopDistance = m_targetUID->m_actorDef->m_physiscRadius + m_controllerActorUID->m_actorDef->m_physiscRadius + m_controllerActorUID->m_actorDef->m_physiscRadius * 0.1f + m_targetUID->m_actorDef->m_physiscRadius * 0.1f;
		if (distanceBetween > stopDistance)
		{
			m_movementSpeed = m_controllerActorUID->m_actorDef->m_runSpeed;
			m_controllerActorUID->MoveInDirection(m_controllerActorUID->GetActorForward(), m_movementSpeed);
		}
		else
		{
			m_controllerActorUID->Attack();
		}
	}
}