#include "Game/Actor.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Game/FishAIController.hpp"
#include "Game/Weapon.hpp"

ActorUID ActorUID::INVALID = ActorUID(0x0000FFFFu, 0x0000FFFFu);
std::vector<ActorDefinition*> ActorDefinition::s_actorDefs;

ActorDefinition::ActorDefinition(XmlElement& element)
	:m_name(ParseXmlAttribute(element, "name", "")),
	m_faction(ParseXmlAttribute(element, "faction", "Neutral")),
	m_health(ParseXmlAttribute(element, "health", 0)),
	m_killMoney(ParseXmlAttribute(element, "killMoney", 0)),
	m_canBePossesed(ParseXmlAttribute(element, "canBePossessed", false)),
	m_corpseLifetime(ParseXmlAttribute(element, "corpseLifetime", 0.0f)),
	m_visible(ParseXmlAttribute(element, "visible", false)),
	m_canKill(ParseXmlAttribute(element, "canKill", true)),
	m_dieOnSpawn(ParseXmlAttribute(element, "dieOnSpawn", false)),
	m_isRenderedOffset(ParseXmlAttribute(element, "isRenderedOffset", false))
{


}

void ActorDefinition::SetCollision(XmlElement& element)
{
	m_physiscHeight = ParseXmlAttribute(element, "height", 0.f);
	m_physiscRadius = ParseXmlAttribute(element, "radius", 0.f);
	m_collideWithWorld = ParseXmlAttribute(element, "collidesWithWorld", false);
	m_collideWithActors = ParseXmlAttribute(element, "collidesWithActors", false);
	m_dieOnCollide = ParseXmlAttribute(element, "dieOnCollide", false);
	m_stopPhysicsOnCollide = ParseXmlAttribute(element, "stopPhysicsOnCollide", false);
	m_damageOnCollide = ParseXmlAttribute(element, "damageOnCollide", FloatRange());
	m_impulseOnCollide = ParseXmlAttribute(element, "impulseOnCollide", 0.f);
}

void ActorDefinition::SetPhysics(XmlElement& element)
{
	m_simulated = ParseXmlAttribute(element, "simulated", false);
	m_walkSpeed = ParseXmlAttribute(element, "walkSpeed", 0.f);
	m_runSpeed = ParseXmlAttribute(element, "runSpeed", 0.f);
	m_drag = ParseXmlAttribute(element, "drag", 0.f);
	m_turnSpeed = ParseXmlAttribute(element, "turnSpeed", 0.f);
	m_flying = ParseXmlAttribute(element, "flying", false);
}

void ActorDefinition::SetCamera(XmlElement& element)
{
	m_eyeHeight = ParseXmlAttribute(element, "eyeHeight", 0.f);
	m_cameraFOV = ParseXmlAttribute(element, "cameraFOV", 60.f);
}

void ActorDefinition::SetAI(XmlElement& element)
{
	m_AIEnabled = ParseXmlAttribute(element, "aiEnabled", false);
	m_AIAngle = ParseXmlAttribute(element, "sightAngle", 0.f);
	m_AISightRadius = ParseXmlAttribute(element, "sightRadius", 0.f);
	m_detectionRate = ParseXmlAttribute(element, "detectionRate", 1.f);
	m_useHeatmap = ParseXmlAttribute(element, "useHeatmap", true);
}

void ActorDefinition::SetVisual(XmlElement& element)
{
	m_size = ParseXmlAttribute(element, "size", Vec2(1.f, 1.f));
	m_pivot = ParseXmlAttribute(element, "pivot", Vec2(0.5f, 0.5f));
	m_billboardType = ParseXmlAttribute(element, "billboardType", "None");
	m_renderLit = ParseXmlAttribute(element, "renderLit", false);
	m_renderRounded = ParseXmlAttribute(element, "renderRounded", false);
	m_shader = g_theRenderer->CreateShader(ParseXmlAttribute(element, "shader", "Data/Shaders/Default").c_str(), VertexType::Vertex_PCUTBN);
	m_spriteSheet = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "spriteSheet", "").c_str());
	m_cellCount = ParseXmlAttribute(element, "cellCount", IntVec2(1, 1));

	XmlElement* animationGroupElement = element.FirstChildElement();
	while (animationGroupElement)
	{
		std::string name = animationGroupElement->Name();
		GUARANTEE_OR_DIE(name == "AnimationGroup", "Root child element in Weapon is in the wrong format");

		SetAnimationGroup(*animationGroupElement);
		animationGroupElement = animationGroupElement->NextSiblingElement();
	}
}

void ActorDefinition::SetAnimationGroup(XmlElement& element)
{
	ActorAnimationGroup* newAnimGroup = new ActorAnimationGroup();
	newAnimGroup->m_animName = ParseXmlAttribute(element, "name", "");
	newAnimGroup->m_scaleBySpeed = ParseXmlAttribute(element, "scaleBySpeed", false);
	newAnimGroup->m_secondsPerFrame = ParseXmlAttribute(element, "secondsPerFrame", 1.f);
	newAnimGroup->m_playbackMode = ParseXmlAttribute(element, "playbackMode", "Once");

	XmlElement* directionElement = element.FirstChildElement();
	while (directionElement)
	{
		std::string name = directionElement->Name();
		GUARANTEE_OR_DIE(name == "Direction", "Root child element in Weapon is in the wrong format");

		SetDirection(*directionElement, *newAnimGroup);
		directionElement = directionElement->NextSiblingElement();
	}
	m_animationGroups.push_back(*newAnimGroup);
}

void ActorDefinition::SetDirection(XmlElement& element, ActorAnimationGroup& group)
{
	group.m_directions.push_back(ParseXmlAttribute(element, "vector", Vec3::ZERO));

	XmlElement* animationElement = element.FirstChildElement();
	while (animationElement)
	{
		std::string name = animationElement->Name();
		GUARANTEE_OR_DIE(name == "Animation", "Root child element in Weapon is in the wrong format");

		SetAnimation(*animationElement, group);
		animationElement = animationElement->NextSiblingElement();
	}
}

void ActorDefinition::SetAnimation(XmlElement& element, ActorAnimationGroup& group)
{
	int startFrame = ParseXmlAttribute(element, "startFrame", 0);
	group.m_startFrame.push_back(startFrame);
	int endFrame = ParseXmlAttribute(element, "endFrame", 0);
	group.m_endFrame.push_back(endFrame);
}

void ActorDefinition::SetSound(XmlElement& element)
{
	XmlElement* soundElement = element.FirstChildElement();
	while (soundElement)
	{
		std::string name = soundElement->Name();
		GUARANTEE_OR_DIE(name == "Sound", "Root child element in Weapon is in the wrong format");

		Sound sound;
		sound.m_name = ParseXmlAttribute(*soundElement, "sound", "");
		sound.m_soundID = g_theAudio->CreateOrGetSound(ParseXmlAttribute(*soundElement, "name", ""), true);
		m_soundList.push_back(sound);

		soundElement = soundElement->NextSiblingElement();
	}

}

void ActorDefinition::SetWeapons(XmlElement& element)
{
	XmlElement* weaponElement = element.FirstChildElement();
	while (weaponElement)
	{
		std::string name = weaponElement->Name();
		GUARANTEE_OR_DIE(name == "Weapon", "Root child element in Weapon is in the wrong format");

		std::string weaponName = ParseXmlAttribute(*weaponElement, "name", "");
		m_inventory.push_back(WeaponDefinition::GetByName(weaponName));
		weaponElement = weaponElement->NextSiblingElement();
	}
}

void ActorDefinition::InitializeActorDefs(char const* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Root Element is null");

	XmlElement* actorDefElement = rootElement->FirstChildElement();

	while (actorDefElement)
	{
		std::string name = actorDefElement->Name();
		GUARANTEE_OR_DIE(name == "ActorDefinition", "Root child element is in the wrong format");

		ActorDefinition* newActorDef = new ActorDefinition(*actorDefElement);

		XmlElement* actorChildElement = actorDefElement->FirstChildElement();
		while (actorChildElement)
		{
			std::string sectionName = actorChildElement->Name();
			if (sectionName == "Collision")
			{
				newActorDef->SetCollision(*actorChildElement);
			}
			if (sectionName == "Physics")
			{
				newActorDef->SetPhysics(*actorChildElement);
			}
			if (sectionName == "Camera")
			{
				newActorDef->SetCamera(*actorChildElement);
			}
			if (sectionName == "AI")
			{
				newActorDef->SetAI(*actorChildElement);
			}
			if (sectionName == "Visuals")
			{
				newActorDef->SetVisual(*actorChildElement);
			}
			if (sectionName == "Sounds")
			{
				newActorDef->SetSound(*actorChildElement);
			}
			if (sectionName == "Inventory")
			{
				newActorDef->SetWeapons(*actorChildElement);
			}
			actorChildElement = actorChildElement->NextSiblingElement();
		}
		s_actorDefs.push_back(newActorDef);
		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

void ActorDefinition::InitializeProjectileActorDefinitions(char const* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Root Element is null");

	XmlElement* actorDefElement = rootElement->FirstChildElement();

	while (actorDefElement)
	{
		std::string name = actorDefElement->Name();
		GUARANTEE_OR_DIE(name == "ActorDefinition", "Root child element is in the wrong format");

		ActorDefinition* newActorDef = new ActorDefinition(*actorDefElement);

		XmlElement* actorChildElement = actorDefElement->FirstChildElement();
		while (actorChildElement)
		{
			std::string sectionName = actorChildElement->Name();
			if (sectionName == "Collision")
			{
				newActorDef->SetCollision(*actorChildElement);
			}
			if (sectionName == "Physics")
			{
				newActorDef->SetPhysics(*actorChildElement);
			}
			if (sectionName == "Visuals")
			{
				newActorDef->SetVisual(*actorChildElement);
			}
			if (sectionName == "Sounds")
			{
				newActorDef->SetSound(*actorChildElement);
			}
			actorChildElement = actorChildElement->NextSiblingElement();
		}
		s_actorDefs.push_back(newActorDef);
		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

void ActorDefinition::ClearDefinition()
{
	for (size_t i = 0; i < s_actorDefs.size(); i++)
	{
		if (s_actorDefs[i] != nullptr)
		{
			delete s_actorDefs[i];
			s_actorDefs[i] = nullptr;
		}
	}
}

ActorDefinition* ActorDefinition::GetByName(std::string const& name)
{
	for (size_t i = 0; i < s_actorDefs.size(); i++)
	{
		if (s_actorDefs[i]->m_name == name)
		{
			return s_actorDefs[i];
		}
	}
	return nullptr;
}

Actor::Actor(ActorDefinition* actorDef, Map* map, unsigned int currentSalt, unsigned int index)
	:m_actorDef(actorDef), m_map(map), m_UID(currentSalt, index), m_owner(ActorUID::INVALID)
{
	for (size_t i = 0; i < actorDef->m_inventory.size(); i++)
	{
		Weapon* newWeapon = new Weapon(actorDef->m_inventory[i]);
		m_weaponInventory.push_back(newWeapon);
	}

	if (actorDef->m_inventory.size() > 0)
	{
		EquipWeapon(0);
	}

	m_currentHealth = m_actorDef->m_health;

	if (m_actorDef->m_spriteSheet)
	{
		m_actorAnimSpriteSheet = new SpriteSheet(*m_actorDef->m_spriteSheet, m_actorDef->m_cellCount);
	}

	m_animationClock = new Clock(*g_theGame->m_clock);

	if (m_actorDef->m_animationGroups.size() > 0)
	{
		PlayAnimationByName(m_actorDef->m_animationGroups[0].m_animName);
	}

	if (m_actorDef->m_dieOnSpawn)
	{
		Die();
	}

	m_isStatic = !m_actorDef->m_simulated;
	m_isFlying = m_actorDef->m_flying;
	m_shrinkTimer = new Timer(0.5f, g_theGame->m_clock);
}

Actor::~Actor()
{
	m_controller = nullptr;
	delete m_AIController;
	m_AIController = nullptr;
}

void Actor::Update()
{
	if (m_controller && !m_controller->IsPlayer())
	{
		m_controller->Update();
	}

	if (m_currentWeapon != nullptr)
	{
		m_currentWeapon->Update();
	}

	UpdateVisuals();

	UpdatePhysics();

	if (m_isDead)
	{
		m_corpseTimer -= g_theGame->m_clock->GetDeltaSeconds();
		if (m_corpseTimer < 0)
		{
			m_isGarbage = true;
		}
	}
}

void Actor::Render(Camera* camera) const
{
	if (!m_actorDef->m_visible)
	{
		return;
	}
	if (!g_currentMap->IsPositionInBound(m_position.x, m_position.y, 0.f))
	{
		return;
	}
	static std::vector<Vertex_PCUTBN> vertexes;
	static std::vector<unsigned int> indexes;
	vertexes.clear();
	indexes.clear();
	vertexes.reserve(6);
	indexes.reserve(12);

	if (m_actorDef->m_faction == "Fish")
	{
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	}
	else
	{
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	}
	g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);



	if (camera)
	{
		Vec3 direction = GetDirectionOfCamera(*camera);
		SpriteAnimDefinition anim = GetSpriteAnimDefBasedOnDirection(direction);
		SpriteDefinition animSprite = anim.GetSpriteDefAtTime(m_animationClock->GetTotalSeconds());
		AABB2 animSpriteUV = animSprite.GetUVs();

		float radius = m_actorDef->m_size.x * m_sizeScale.x;
		float height = m_actorDef->m_size.y * m_sizeScale.y;

		float offsetHeight = 0.f;

		if (g_currentMap->IsTileInBoundFromPos(m_position))
		{
			if (g_currentMap->GetTileFromPos(m_position)->m_tileDef->m_isWater && m_actorDef->m_isRenderedOffset && !m_isFlying)
			{
				offsetHeight = Interpolate(0.f, -0.2f, 
					SmoothStop4(RangeMapClamped(m_shrinkTimer->GetElapsedFraction(), 0.f, m_shrinkTimer->m_period, 0.f, 1.f)));
			}
		}

		Vec3 alignment = Vec3(0, radius * m_actorDef->m_pivot.x, height * m_actorDef->m_pivot.y);

		Vec3 BL = Vec3(0, 0, offsetHeight) - alignment;
		Vec3 BR = Vec3(0, radius, offsetHeight) - alignment;
		Vec3 TL = Vec3(0, 0, height + offsetHeight) - alignment;
		Vec3 TR = Vec3(0, radius, height + offsetHeight) - alignment;

		if (m_actorDef->m_renderRounded)
		{
			AddVertsForRoundedQuad3D(vertexes, indexes, BL, BR, TL, TR, Rgba8::COLOR_WHITE, animSpriteUV);
		}
		else
		{
			AddVertsForQuad3D(vertexes, indexes, BL, BR, TL, TR, Rgba8::COLOR_WHITE, animSpriteUV);
		}

		BilboardType type = BilboardType::NONE;
		if (m_actorDef->m_billboardType == "WorldUpFacing")
		{
			type = BilboardType::WORLD_UP_CAMERA_FACING;
		}
		else if (m_actorDef->m_billboardType == "WorldUpOpposing")
		{
			type = BilboardType::WORLD_UP_CAMERA_OPPOSING;
		}
		else if (m_actorDef->m_billboardType == "FullOpposing")
		{
			type = BilboardType::FULL_CAMERA_OPPOSING;
		}
		Mat44 billBoard = GetBillboardMatrix(type, camera->GetModelMatrix(), m_position);

		if (m_actorDef->m_faction == "Fish")
		{
			float angle = Vec2(GetActorForward().x, GetActorForward().y).GetOrientationDegrees();
			billBoard.AppendZRotation(angle + 90);
		}
		g_theRenderer->SetModelConstants(billBoard, m_actorDef->m_solidColor);
		g_theRenderer->BindTexture(m_actorDef->m_spriteSheet);
	}
	else
	{
		g_theRenderer->SetModelConstants(GetModelMatrix(), m_actorDef->m_solidColor);
		g_theRenderer->BindTexture(nullptr);
	}

	if (m_actorDef->m_renderLit)
	{
		g_theRenderer->BindShader(m_actorDef->m_shader, VertexType::Vertex_PCUTBN);
	}
	else
	{
		g_theRenderer->BindShader(nullptr, VertexType::Vertex_PCU);
	}

	g_theRenderer->SetLightConstants(Vec3(g_currentMap->m_sunX, g_currentMap->m_sunY, -1), g_currentMap->m_sunIntensity, g_currentMap->m_ambIntensity);

	g_theRenderer->DrawIndexedBuffer(vertexes, indexes);


	// 	std::vector<Vertex_PCUTBN> debugvertexes;
	// 	std::vector<unsigned int> debugindexes;
	// 
	// 	AddVertsForCylinder3D(debugvertexes, debugindexes, Vec3::ZERO, Vec3(0, 0, m_actorDef->m_physiscHeight), m_actorDef->m_physiscRadius);
	// 
	// 	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	// 	g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);
	// 	g_theRenderer->SetModelConstants(GetModelMatrix(), m_tint);
	// 	g_theRenderer->BindTexture(nullptr);
	// 	g_theRenderer->BindShader(nullptr);
	// 	g_theRenderer->DrawIndexedBuffer(debugvertexes, debugindexes);
}

void Actor::Die()
{
	if (m_isDead)
	{
		return;
	}
	m_isDead = true;
	m_corpseTimer = m_actorDef->m_corpseLifetime;
	PlayAnimationByName("Death");
	PlaySoundByName("Death");
}


Mat44 Actor::GetModelMatrix() const
{
	Mat44 modelMat = Mat44();
	modelMat = m_orientationDegrees.GetAsMatrix_IFwd_JLeft_KUp();
	modelMat.SetTranslation3D(m_position);
	return modelMat;
}

IntVec2 Actor::GetTileLocation() const
{
	return IntVec2((int)m_position.x, (int)m_position.y);
}

Vec2 Actor::GetColliderCenterXY()
{
	return Vec2(m_position.x, m_position.y);
}

FloatRange Actor::GetColliderMinMaxZ()
{
	return FloatRange(m_position.z, m_position.z + m_actorDef->m_physiscHeight);
}

Vec3 Actor::GetControllerForward() const
{
	if (m_controller->IsPlayer())
	{
		return reinterpret_cast<PlayerController*>(m_controller)->m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
	}
	else
	{
		return m_orientationDegrees.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
	}
}

Vec3 Actor::GetActorForward() const
{
	return m_orientationDegrees.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
}

void Actor::SetActorPosition(Vec2 centerXY, FloatRange minMaxZ)
{
	m_position = Vec3(centerXY.x, centerXY.y, minMaxZ.m_min);
}

void Actor::UpdatePhysics()
{
	if (m_isDead)
	{
		return;
	}
	if (m_isStatic)
	{
		return;
	}

	if (m_actorDef->m_hasGravity)
	{
		AddForce(Vec3(0.f, 0.f, -1.f), 9.8f * m_gravityModifier);
	}

	float drag = m_actorDef->m_drag * -1.f;
	AddForce(m_velocity, drag);

	if (m_hasGravity)
	{
		AddForce(Vec3(0, 0, -1), 9.8f);
	}

	m_velocity += m_acceleration * g_theGame->m_clock->GetDeltaSeconds();
	m_position += m_velocity * g_theGame->m_clock->GetDeltaSeconds();

	m_acceleration = Vec3::ZERO;

	if (!m_actorDef->m_flying)
	{
		m_position.z = 0;
	}
}

void Actor::UpdateVisuals()
{
	if (m_isDead)
	{
		return;
	}
	if (!m_actorDef->m_visible)
	{
		return;
	}

	if (m_currentAnimation->m_scaleBySpeed)
	{
		m_animationClock->SetTimeScale(m_velocity.GetLength() / m_actorDef->m_runSpeed);
	}
	else
	{
		m_animationClock->SetTimeScale(1);
	}
}

void Actor::TakeDamage(int damage, ActorUID damageDealer)
{
	if (m_isDead || !m_actorDef->m_canKill)
	{
		return;
	}
	m_currentHealth -= damage;
	if (m_currentHealth <= 0)
	{
		m_currentHealth = 0;
		Die();
	}

	if (m_controller)
	{
		if (damageDealer->m_owner.IsValid())
		{
			m_controller->DamagedBy(damageDealer->m_owner);
		}
		else
		{
			m_controller->DamagedBy(damageDealer);
		}
	}
}

void Actor::AddForce(Vec3 dir, float strength)
{
	m_acceleration += dir * strength;
}

void Actor::AddImpulse(Vec3 dir, float strength)
{
	m_velocity += dir * strength;
}

void Actor::OnCollide(Actor* collidedActor)
{
	if (m_actorDef->m_dieOnCollide)
	{
		Die();
	}
	if (m_actorDef->m_stopPhysicsOnCollide)
	{
		m_isStatic = true;
		m_isFlying = false;
		m_shrinkTimer->Start();
	}
	if (g_currentMap->IsTileInBoundFromPos(m_position))
	{
		if (g_currentMap->GetTileFromPos(m_position)->m_tileDef->m_isWater && m_actorDef->m_faction != "Fish")
		{
			ActorUID waterDrop = g_currentMap->SpawnActor("WaterDrop", m_position, EulerAngles(), Rgba8(200, 200, 200));
			waterDrop->m_sizeScale = Vec2(3.f, 3.f);
			PlaySoundByName("Hit");
		}
	}
	if (collidedActor)
	{
		AddImpulse(Vec3(m_position - collidedActor->m_position).GetNormalized(), collidedActor->m_actorDef->m_impulseOnCollide);
		if (m_actorDef->m_faction != collidedActor->m_actorDef->m_faction)
		{
			if (m_actorDef->m_damageOnCollide.m_max != 0)
			{
				int damageFromA = g_theRNG->RollRandomIntInRange((int)m_actorDef->m_damageOnCollide.m_min, (int)m_actorDef->m_damageOnCollide.m_max);
				collidedActor->TakeDamage(damageFromA, m_UID);
			}
		}
	}
}

void Actor::OnPossessed(Controller* controller)
{
	m_controller = controller;
	if (!controller->IsPlayer() && m_actorDef->m_AIEnabled)
	{
		m_AIController = reinterpret_cast<AIController*>(m_controller);
	}
}

void Actor::OnUnpossessed()
{
	if (m_controller != nullptr)
	{
		m_controller = nullptr;
	}
	if (m_actorDef->m_AIEnabled)
	{
		m_controller = m_AIController;
	}
}

void Actor::MoveInDirection(Vec3 dir, float speed)
{
	AddForce(dir, m_actorDef->m_drag * speed);
	if (m_animationClock->GetTotalSeconds() >= m_currentAnimation->m_duration)
	{
		PlayAnimationByName("Walk");
	}
}

void Actor::TurnInDirection(float goalAngle, float maxDeltaAngle)
{
	m_orientationDegrees.m_yawDegrees = GetTurnedTowardDegrees(m_orientationDegrees.m_yawDegrees, goalAngle, maxDeltaAngle);
}

void Actor::Attack()
{
	m_currentWeapon->Fire();
}

void Actor::EquipWeapon(int index)
{
	if (m_currentWeapon && m_currentWeapon->m_isFishingRod)
	{
		if (g_currentMap->GetActorByUID(m_currentWeapon->m_bait))
		{
			m_currentWeapon->m_bait->Die();
			m_currentWeapon->m_bait = ActorUID::INVALID;
		}
		m_currentWeapon->StopAllFishingTimer();
		m_currentWeapon->m_isCurrentlyFishing = false;
	}
	m_currentWeaponIndex = index;
	m_currentWeapon = m_weaponInventory[index];
	if (m_currentWeapon->m_holder != m_UID)
	{
		m_currentWeapon->m_holder = m_UID;
	}
	m_currentWeapon->PlayAnimationByName("Idle");
}


void Actor::PlayAnimationByName(std::string name)
{
	if (!m_actorDef->m_visible)
	{
		return;
	}
	if (!m_currentAnimation)
	{
		m_currentAnimation = &m_actorDef->m_animationGroups[0];
	}
	if (m_currentAnimation->m_animName == name)
	{
		if (m_currentAnimation->m_playbackMode == "Once" && m_animationClock->GetTotalSeconds() < m_currentAnimation->m_duration)
		{
			return;
		}
	}
	for (size_t i = 0; i < m_actorDef->m_animationGroups.size(); i++)
	{
		if (m_actorDef->m_animationGroups[i].m_animName == name)
		{
			m_currentAnimation = &m_actorDef->m_animationGroups[i];
			break;
		}
	}
	m_currentAnimation->m_duration = (m_currentAnimation->m_endFrame[0] - m_currentAnimation->m_startFrame[0] + 1) * m_currentAnimation->m_secondsPerFrame;
	if (m_currentAnimation->m_playbackMode == "Once")
	{
		m_animationClock->Reset();
	}
}

void Actor::PlaySoundByName(std::string name, bool overrideTheCurrentSound, float volumne, float speed, float balance)
{
	if (!overrideTheCurrentSound)
	{
		if (g_theAudio->IsPlaying(m_currentSound.m_soundPlaybackID) && m_currentSound.m_name == name)
		{
			return;
		}
		else
		{
			g_theAudio->StopSound(m_currentSound.m_soundPlaybackID);
		}
	}

	for (size_t i = 0; i < m_actorDef->m_soundList.size(); i++)
	{
		if (m_actorDef->m_soundList[i].m_name == name)
		{
			m_currentSound.m_name = name;
			m_currentSound.m_soundPlaybackID = g_theAudio->StartSoundAt(m_actorDef->m_soundList[i].m_soundID, m_position, false, volumne * g_gameConfigBlackboard.GetValue("sfxVolume", 1.0f), balance, speed);
			break;
		}
	}
}

Weapon* Actor::HasWeapon(std::string name) const
{
	for (size_t i = 0; i < m_weaponInventory.size(); i++)
	{
		if (m_weaponInventory[i]->m_weaponDef->m_name == name)
		{
			return m_weaponInventory[i];
		}
	}
	return nullptr;
}

Vec3 Actor::GetDirectionOfCamera(Camera& camera) const
{
	if (m_currentAnimation->m_animName == "Death" || m_currentAnimation->m_animName == "Idle")
	{
		return Vec3(1, 0, 0);
	}
	Vec2 camPos = camera.GetModelMatrix().GetTranslation2D();
	Vec2 toCamPosNormalized = (camPos - Vec2(m_position.x, m_position.y)).GetNormalized();
	Vec3 result = Vec3(toCamPosNormalized.x, toCamPosNormalized.y, 0);
	Mat44 actorMatrixWToL = GetModelMatrix().GetOrthonormalInverse();

	return actorMatrixWToL.TransformVectorQuantity3D(result) * -1.f;
}

SpriteAnimDefinition Actor::GetSpriteAnimDefBasedOnDirection(Vec3 direction) const
{
	float biggestDotProduct = 0.f;
	int index = 0;

	for (size_t i = 0; i < m_currentAnimation->m_directions.size(); i++)
	{
		float dot = DotProduct3D(direction, m_currentAnimation->m_directions[i].GetNormalized());
		if (dot > biggestDotProduct)
		{
			biggestDotProduct = dot;
			index = (int)i;
		}
	}
	SpriteAnimPlaybackType playbackMode = SpriteAnimPlaybackType::ONCE;
	if (m_currentAnimation->m_playbackMode == "Loop")
	{
		playbackMode = SpriteAnimPlaybackType::LOOP;
	}
	if (m_currentAnimation->m_playbackMode == "PingPong")
	{
		playbackMode = SpriteAnimPlaybackType::PINGPONG;
	}

	return SpriteAnimDefinition(*m_actorAnimSpriteSheet, m_currentAnimation->m_startFrame[index], m_currentAnimation->m_endFrame[index], m_currentAnimation->m_duration, playbackMode);
}

ActorUID::ActorUID(unsigned int salt, unsigned int index)
{
	m_data = (salt << 16) | (index & 0xFFFF);
}

int ActorUID::GetIndex() const
{
	return m_data & 0xFFFF;
}

bool ActorUID::IsValid() const
{
	return m_data != ActorUID::INVALID.m_data;
}

Actor* ActorUID::GetActor() const
{
	return g_currentMap->m_allActorsList[GetIndex()];
}

Actor* ActorUID::operator->() const
{
	return GetActor();
}

bool ActorUID::operator==(ActorUID otherUID) const
{
	return m_data == otherUID.m_data;
}

bool ActorUID::operator!=(ActorUID otherUID) const
{
	return m_data != otherUID.m_data;
}