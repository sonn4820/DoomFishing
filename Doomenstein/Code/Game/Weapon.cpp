#include "Game/Weapon.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerController.hpp"
#include "Engine/Math/Spline.hpp"


std::vector<WeaponDefinition*> WeaponDefinition::s_weaponDefs;

WeaponDefinition::WeaponDefinition(XmlElement& element)
	:m_name(ParseXmlAttribute(element, "name", "")),
	m_refireTime(ParseXmlAttribute(element, "refireTime", 0.f)),
	m_magazine(ParseXmlAttribute(element, "magazine", 0)),
	m_price(ParseXmlAttribute(element, "price", 0)),
	m_magPrice(ParseXmlAttribute(element, "magPrice", 0)),
	m_moveSpeed(ParseXmlAttribute(element, "moveSpeed", 1.f)),
	m_canScope(ParseXmlAttribute(element, "canScope", false)),
	m_hasGunSmoke(ParseXmlAttribute(element, "gunSmoke", false)),
	m_isFlash(ParseXmlAttribute(element, "isFlash", true)),
	m_smokeRadius(ParseXmlAttribute(element, "smokeRadius", 0.025f)),
	m_smokeSpeed(ParseXmlAttribute(element, "smokeSpeed", 1.f)),
	m_smokeOffset(ParseXmlAttribute(element, "smokeOffset", Vec3())),
	m_rayCount(ParseXmlAttribute(element, "rayCount", 0.f)),
	m_rayCone(ParseXmlAttribute(element, "rayCone", 0.f)),
	m_rayRange(ParseXmlAttribute(element, "rayRange", 0.f)),
	m_rayDamage(ParseXmlAttribute(element, "rayDamage", FloatRange(0.f, 0.f))),
	m_rayImpulse(ParseXmlAttribute(element, "rayImpulse", 0.f)),
	m_projectileCount(ParseXmlAttribute(element, "projectileCount", 0)),
	m_projectileCone(ParseXmlAttribute(element, "projectileCone", 0.f)),
	m_projectileSpeed(ParseXmlAttribute(element, "projectileSpeed", 0.f)),
	m_meleeCount(ParseXmlAttribute(element, "meleeCount", 0)),
	m_meleeArc(ParseXmlAttribute(element, "meleeArc", 0.f)),
	m_meleeRange(ParseXmlAttribute(element, "meleeRange", 0.f)),
	m_meleeDamage(ParseXmlAttribute(element, "meleeDamage", FloatRange(0.f, 0.f))),
	m_meleeImpulse(ParseXmlAttribute(element, "meleeImpulse", 0.f))
{
	m_projectileActor = ActorDefinition::GetByName(ParseXmlAttribute(element, "projectileActor", ""));
	if (m_canScope)
	{
		m_scopeTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "scopeTexture", "").c_str());
	}
	if (m_hasGunSmoke)
	{
		m_smokeTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "smokeTexture", "").c_str());
	}
}

void WeaponDefinition::SetHUD(XmlElement& element)
{
	m_shader = g_theRenderer->CreateShader(ParseXmlAttribute(element, "shader", "Data/Shaders/Default").c_str());
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "baseTexture", "Data/Images/Hud_Base.png").c_str());
	m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "reticleTexture", "Data/Images/Reticle.png").c_str());
	m_reticleSize = ParseXmlAttribute(element, "reticleSize", IntVec2(16, 16));
	m_spriteSize = ParseXmlAttribute(element, "spriteSize", IntVec2());
	m_spritePivot = ParseXmlAttribute(element, "spritePivot", Vec2());

	XmlElement* animationElement = element.FirstChildElement();
	while (animationElement)
	{
		std::string name = animationElement->Name();
		GUARANTEE_OR_DIE(name == "Animation", "Root child element in Weapon is in the wrong format");

		SetAnimation(*animationElement);
		animationElement = animationElement->NextSiblingElement();
	}

	m_weaponSpriteSheet = new SpriteSheet(*m_animation[0].m_spriteSheet, m_animation[0].m_cellCount);
}

void WeaponDefinition::SetAnimation(XmlElement& element)
{
	WeaponAnimation newAnim;
	newAnim.m_name = ParseXmlAttribute(element, "name", "");
	newAnim.m_animShader = g_theRenderer->CreateShader(ParseXmlAttribute(element, "shader", "Data/Shaders/Default").c_str(), VertexType::Vertex_PCUTBN);
	newAnim.m_spriteSheet = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "spriteSheet", "").c_str());
	newAnim.m_cellCount = ParseXmlAttribute(element, "cellCount", IntVec2());
	newAnim.m_startFrame = ParseXmlAttribute(element, "startFrame", 0);
	newAnim.m_endFrame = ParseXmlAttribute(element, "endFrame", 0);
	newAnim.m_duration = (newAnim.m_endFrame - newAnim.m_startFrame + 1) * ParseXmlAttribute(element, "secondsPerFrame", 0.f);
	newAnim.m_playbackMode = ParseXmlAttribute(element, "playbackMode", "Once");
	m_animation.push_back(newAnim);
}

void WeaponDefinition::SetSound(XmlElement& element)
{
	XmlElement* soundElement = element.FirstChildElement();
	while (soundElement)
	{
		std::string name = soundElement->Name();
		GUARANTEE_OR_DIE(name == "Sound", "Root child element in Weapon is in the wrong format");

		m_sound.push_back(ParseXmlAttribute(*soundElement, "sound", ""));
		m_soundName.push_back(ParseXmlAttribute(*soundElement, "name", ""));
		m_overrideCurrentSound = ParseXmlAttribute(*soundElement, "override", true);

		soundElement = soundElement->NextSiblingElement();
	}
}

void WeaponDefinition::InitializeWeaponDefs(char const* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Root Element is null");

	XmlElement* weaponDefElement = rootElement->FirstChildElement();

	while (weaponDefElement)
	{
		std::string name = weaponDefElement->Name();
		GUARANTEE_OR_DIE(name == "WeaponDefinition", "Root child element is in the wrong format");
		WeaponDefinition* newWeaponDef = new WeaponDefinition(*weaponDefElement);

		XmlElement* weaponChildElement = weaponDefElement->FirstChildElement();
		while (weaponChildElement)
		{
			std::string sectionName = weaponChildElement->Name();

			if (sectionName == "HUD")
			{
				newWeaponDef->SetHUD(*weaponChildElement);
			}
			if (sectionName == "Sounds")
			{
				newWeaponDef->SetSound(*weaponChildElement);
			}

			weaponChildElement = weaponChildElement->NextSiblingElement();
		}

		s_weaponDefs.push_back(newWeaponDef);
		weaponDefElement = weaponDefElement->NextSiblingElement();
	}
}

WeaponDefinition* WeaponDefinition::GetByName(std::string const& name)
{
	for (size_t i = 0; i < s_weaponDefs.size(); i++)
	{
		if (s_weaponDefs[i]->m_name == name)
		{
			return s_weaponDefs[i];
		}
	}
	return nullptr;
}

void WeaponDefinition::ClearDefinition()
{
	for (size_t i = 0; i < s_weaponDefs.size(); i++)
	{
		if (s_weaponDefs[i] != nullptr)
		{
			delete s_weaponDefs[i];
			s_weaponDefs[i] = nullptr;
		}
	}
}

Weapon::Weapon(WeaponDefinition* weaponDef)
	:m_weaponDef(weaponDef), m_holder(ActorUID::INVALID)
{
	for (size_t i = 0; i < m_weaponDef->m_sound.size(); i++)
	{
		Sound newSound;
		newSound.m_name = m_weaponDef->m_sound[i];
		newSound.m_soundID = g_theAudio->CreateOrGetSound(m_weaponDef->m_soundName[i], true);
		m_soundsList.push_back(newSound);
	}

	m_animationClock = new Clock(*g_theGame->m_clock);
	m_smokeTimer = new Timer(0.15f, g_theGame->m_clock);
	m_reloadingTimer = new Timer(2.f, g_theGame->m_clock);

	m_currentMagCount = m_weaponDef->m_magazine;
	m_soundOverrided = m_weaponDef->m_overrideCurrentSound;

	if (m_weaponDef->m_name == "FishingRod")
	{
		m_isFishingRod = true;	
		m_fishingThrowTimer = new Timer(2.f, g_theGame->m_clock);
		m_fishingPullTimer = new Timer(1.5f, g_theGame->m_clock);
	}
}

Weapon::~Weapon()
{

}

void Weapon::Update()
{
	m_refireTimer -= g_theGame->m_clock->GetDeltaSeconds();
	if (m_smokeTimer->HasPeriodElapsed())
	{
		m_smokeTimer->Stop();
	}
	if (m_isFishingRod)
	{
		if (m_fishingThrowTimer->HasPeriodElapsed())
		{
			m_fishingThrowTimer->Stop();
		}
		if (m_fishingPullTimer->HasPeriodElapsed())
		{
			m_fishingPullTimer->Stop();
			m_isCurrentlyFishing = false;
		}
		else if (m_fishingPullTimer->GetElapsedFraction() > 0.85f)
		{
			if (m_bait.IsValid())
			{
				m_bait->Die();
				m_bait = ActorUID::INVALID;
			}
		}
		if (m_bait.IsValid())
		{
			if (m_bait->m_position.IsDifferent(m_holder->m_position, 15.5f))
			{
				m_fishingThrowTimer->Stop();
				m_fishingPullTimer->Stop();
				m_isCurrentlyFishing = false;
				if (m_bait.IsValid())
				{
					m_bait->Die();
					m_bait = ActorUID::INVALID;
				}
			}
		}

	}
	if (!HasAmmo())
	{
		PlayAnimationByName("Idle");
		m_isReloading = false;
		return;
	}
	if (m_animationClock->GetTotalSeconds() > m_currentAnimation.m_duration)
	{
		if (m_isReloading)
		{
			PlayAnimationByName("Reload");
		}
		else
		{
			PlayAnimationByName("Idle");
		}
	}
	if (m_reloadingTimer->HasPeriodElapsed())
	{
		m_reloadingTimer->Stop();
		m_isReloading = false;
		AddBulletFromTotalToMag();
	}
	if (m_weaponDef->m_hasGunSmoke)
	{
		float distanceSquared = (m_lastHitPos - m_lastSmokeStartPos).GetLengthSquared();
		if (distanceSquared > 0.f)
		{
			m_lastSmokeStartPos += (m_lastHitPos - m_lastSmokeStartPos).GetNormalized() * m_weaponDef->m_smokeSpeed * SmoothStop6(m_smokeTimer->GetElapsedFraction()) * g_theGame->m_clock->GetDeltaSeconds(); //*(1.f - 1.f / distance)
		}
	}
}

void Weapon::Render(Camera& camera) const
{
	UNUSED(camera);
	if (m_holder->m_isDead)
	{
		return;
	}
	if (m_weaponDef->m_hasGunSmoke && !m_smokeTimer->IsStopped() && (m_lastHitPos - m_lastSmokeStartPos).GetLengthSquared() > 1.f)
	{
		if (m_weaponDef->m_isFlash)
		{
			g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		}
		else
		{
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		}

		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);
		g_theRenderer->SetLightConstants(Vec3(g_currentMap->m_sunX, g_currentMap->m_sunY, -1), g_currentMap->m_sunIntensity, g_currentMap->m_ambIntensity);
		g_theRenderer->BindTexture(m_weaponDef->m_smokeTexture);
		g_theRenderer->BindShader(nullptr);

		std::vector<Vertex_PCU> vertexes;
		std::vector<unsigned int> indexes;


		AddVertsForCylinder3DNoCap(vertexes, indexes, m_lastSmokeStartPos, m_lastHitPos, m_weaponDef->m_smokeRadius, Rgba8::COLOR_WHITE);

		Rgba8 color = Interpolate(Rgba8(255, 255, 255, 140), Rgba8(255, 255, 255, 0), SmoothStart3(m_smokeTimer->GetElapsedFraction()));
		g_theRenderer->SetModelConstants(Mat44(), color);

		g_theRenderer->DrawIndexedBuffer(vertexes, indexes);
	}

	if (m_isFishingRod)
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);
		g_theRenderer->SetLightConstants(Vec3(g_currentMap->m_sunX, g_currentMap->m_sunY, -1), g_currentMap->m_sunIntensity, g_currentMap->m_ambIntensity);
		g_theRenderer->BindShader(g_currentMap->m_shader, VertexType::Vertex_PCUTBN);

		std::vector<Vertex_PCUTBN> lineVerts;
		std::vector<unsigned int> lineIndexes;
		Vec3 eyePosition = Vec3(m_holder->m_position.x, m_holder->m_position.y, m_holder->m_position.z + m_holder->m_actorDef->m_eyeHeight)
			+ m_holder->GetControllerForward() * m_weaponDef->m_smokeOffset.x
			+ m_holder->GetModelMatrix().GetJBasis3D() * m_weaponDef->m_smokeOffset.y
			+ m_holder->GetModelMatrix().GetKBasis3D() * m_weaponDef->m_smokeOffset.z;

		Vec3 fishingRodEndRodAnim = Vec3(0.6f, -0.2f, 0.08f);
		if (!m_fishingThrowTimer->IsStopped())
		{
			fishingRodEndRodAnim = Interpolate(Vec3(-0.1f, -0.4f, 0.7f), Vec3(0.6f, -0.2f, 0.08f),
				SmoothStop6(Clamp(m_fishingThrowTimer->GetElapsedFraction(), 0.f, 1.f)));
		}
		if (!m_fishingPullTimer->IsStopped())
		{
			fishingRodEndRodAnim = Interpolate(Vec3(0.6f, -0.2f, 0.08f), Vec3(-0.1f, -0.4f, 0.2f),
				SmoothStop6(Clamp(m_fishingPullTimer->GetElapsedFraction(),0.f, 1.f)));
		}
		Mat44 eyePosMat = reinterpret_cast<PlayerController*>(m_holder->m_controller)->m_playerCamera->GetModelMatrix();
		eyePosMat.AppendTranslation3D(fishingRodEndRodAnim);
		Vec3 endRodPoint = eyePosMat.GetTranslation3D();

		AddVertsForCylinder3D(lineVerts, lineIndexes, eyePosition, endRodPoint, 0.006f);

		g_theRenderer->BindTexture(g_theGame->m_fishingRodTexture);
		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawIndexedBuffer(lineVerts, lineIndexes);

		std::vector<Vertex_PCUTBN> baitVerts;
		std::vector<unsigned int> baitIndexes;

		if (m_bait.IsValid())
		{
			float offsetHeight = 0.f;

			if (g_currentMap->IsTileInBoundFromPos(m_bait->m_position))
			{
				if (g_currentMap->GetTileFromPos(m_bait->m_position)->m_tileDef->m_isWater && m_bait->m_actorDef->m_isRenderedOffset && !m_bait->m_isFlying)
				{
					offsetHeight = Interpolate(0.0f, -0.2f,
						SmoothStop4(RangeMapClamped(m_bait->m_shrinkTimer->GetElapsedFraction(), 0.f, m_bait->m_shrinkTimer->m_period, 0.f, 1.f)));
				}
			}

			Vec3 renderPosition = m_bait->m_position;
			renderPosition.z += offsetHeight;
			Vec3 toHit = renderPosition - eyePosMat.GetTranslation3D();
			CubicHermiteCurve3D curve = CubicHermiteCurve3D(eyePosMat.GetTranslation3D(), Vec3(0.f, 0.f, toHit.z * -0.15f), renderPosition, Vec3(0.f, 0.f, toHit.z * 0.3f));
			int num = 128;
			for (size_t i = 0; i < num; i++)
			{
				Vec3 pos1 = curve.EvaluateAtParametric((float)i / (float)num);
				Vec3 pos2 = curve.EvaluateAtParametric(((float)i + 1) / (float)num);
				AddVertsForCylinder3D(baitVerts, baitIndexes, pos1, pos2, 0.002f, Rgba8::COLOR_GRAY);
			}
		}
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawIndexedBuffer(baitVerts, baitIndexes);
	}
}

void Weapon::Fire()
{
	if ( !HasAmmo() || m_isReloading || !m_reloadingTimer->IsStopped() || m_refireTimer > 0.f)
	{
		return;
	}
	m_holder->PlayAnimationByName("Attack");
	PlaySoundByName("Fire", m_weaponDef->m_overrideCurrentSound);
	PlayAnimationByName("Attack");

	if (m_weaponDef->m_magazine != -1)
	{
		m_currentMagCount--;
		if (m_currentMagCount <= 0)
		{
			Reload();
		}
	}

	Vec3 eyePosition = Vec3(m_holder->m_position.x, m_holder->m_position.y, m_holder->m_position.z + m_holder->m_actorDef->m_eyeHeight);

	if (m_weaponDef->m_hasGunSmoke)
	{
		m_smokeTimer->Start();
	}

	for (int i = 0; i < m_weaponDef->m_rayCount; i++)
	{
		Vec3 hitForward;

		if (m_weaponDef->m_canScope && !m_isScoping)
		{
			hitForward = GetRandomDirectionInCone(m_weaponDef->m_rayCone + g_theRNG->RollRandomFloatInRange(5.f, 10.f));
		}
		else
		{
			hitForward = GetRandomDirectionInCone(m_weaponDef->m_rayCone);
		}


		RaycastResult closestHit = m_holder->m_map->RaycastAll(eyePosition, hitForward, m_weaponDef->m_rayRange, m_holder);

		if (closestHit.m_didImpact)
		{
			Vec3 toClosestHitDir = (closestHit.m_rayStartPos - closestHit.m_impactPos).GetNormalized();

			if (closestHit.m_targetUID.IsValid() && closestHit.m_targetUID != m_holder)
			{
				if (closestHit.m_targetUID->m_actorDef->m_dieOnCollide
					|| closestHit.m_targetUID->m_actorDef->m_dieOnSpawn
					|| !closestHit.m_targetUID->m_actorDef->m_canKill)
				{
					closestHit.m_didImpact = false;
					return;
				}
				else
				{
					int damage = g_theRNG->RollRandomIntInRange((int)m_weaponDef->m_rayDamage.m_min, (int)m_weaponDef->m_rayDamage.m_max);
					closestHit.m_targetUID->TakeDamage(damage, m_holder);
					closestHit.m_targetUID->AddImpulse((closestHit.m_targetUID->m_position - m_holder->m_position).GetNormalized(), m_weaponDef->m_rayImpulse);

					ActorUID bloodHit = g_currentMap->SpawnActor("BloodSplatter", closestHit.m_impactPos);
					bloodHit->m_sizeScale = Vec2(Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f), Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f));

				}
			}
			else
			{
				if (g_currentMap->GetTile(closestHit.m_impactTileCoord)->m_tileDef->m_isWater)
				{
					ActorUID waterDrop = g_currentMap->SpawnActor("WaterDrop", closestHit.m_impactPos, EulerAngles(), Rgba8(200, 200, 200));
					waterDrop->m_sizeScale = Vec2(Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f), Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f));
				}
				else
				{
					if (closestHit.m_impactPos.z < 4.5f)
					{
						if (closestHit.m_impactPos.z > 0.15f)
						{
							ActorUID bulletHit = g_currentMap->SpawnActor("BulletHit", closestHit.m_impactPos + toClosestHitDir * 0.1f);
							bulletHit->m_sizeScale = Vec2(Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f), Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f));
						}
						else
						{
							ActorUID bulletHit = g_currentMap->SpawnActor("BulletHit", closestHit.m_impactPos);
							bulletHit->m_sizeScale = Vec2(Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f), Clamp(m_weaponDef->m_rayImpulse / 4.5f, 1.f, 3.f));
						}
					}
				}

			}
			m_lastHitPos = closestHit.m_impactPos;
			m_lastSmokeStartPos = eyePosition + m_holder->GetActorForward() * m_weaponDef->m_smokeOffset.x
				+ m_holder->GetModelMatrix().GetJBasis3D() * m_weaponDef->m_smokeOffset.y
				+ m_holder->GetModelMatrix().GetKBasis3D() * m_weaponDef->m_smokeOffset.z;
		}
		else
		{
			m_lastSmokeStartPos = eyePosition + m_holder->GetActorForward() * m_weaponDef->m_smokeOffset.x
				+ m_holder->GetModelMatrix().GetJBasis3D() * m_weaponDef->m_smokeOffset.y
				+ m_holder->GetModelMatrix().GetKBasis3D() * m_weaponDef->m_smokeOffset.z;
			m_lastHitPos = m_lastSmokeStartPos + hitForward.GetNormalized() * 20.f;
		}
	}
	if (m_isFishingRod)
	{
		if (m_fishingThrowTimer->IsStopped())
		{
			if (m_bait.IsValid())
			{
				m_bait->Die();
				m_bait = ActorUID::INVALID;
			}

			m_isCurrentlyFishing = true;
			m_lastHolderFishingPos = m_holder->m_position;

			m_bait = g_currentMap->SpawnActor("Bait", eyePosition + m_holder->GetControllerForward() * -0.02f - m_holder->GetModelMatrix().GetJBasis3D() * 0.2f, EulerAngles(), Rgba8::COLOR_WHITE, Rgba8::COLOR_WHITE, m_holder);
			m_bait->m_hasGravity = true;
			m_bait->m_gravityModifier = 1.f;
			Mat44 transform = reinterpret_cast<PlayerController*>(m_holder->m_controller)->m_playerCamera->GetModelMatrix();
			transform.AppendTranslation3D(Vec3(0, 0, 2));
			transform.AppendYRotation(-25.f);
			transform.AppendZRotation(2.5f);
			m_bait->AddImpulse(transform.GetIBasis3D(), 6 + m_holder->m_velocity.GetLength() * 
				DotProduct3D(m_holder->GetActorForward(), m_holder->m_velocity)/ fabsf(DotProduct3D(m_holder->GetActorForward(), m_holder->m_velocity)));

			m_fishingThrowTimer->Start();
		}
	}
	if (m_currentMagCount == 0)
	{
		m_lastSmokeStartPos = Vec3::ZERO;
		m_lastHitPos = Vec3::ZERO;
	}
	for (int i = 0; i < m_weaponDef->m_projectileCount; i++)
	{
		Vec3 offset = Vec3(0.f, 0.f, 0.1f);
		ActorUID projectile = g_currentMap->SpawnActor(m_weaponDef->m_projectileActor->m_name, eyePosition + m_holder->GetControllerForward() * 0.5f - offset, EulerAngles(), Rgba8::COLOR_WHITE, Rgba8::COLOR_WHITE, m_holder);
		projectile->AddImpulse(GetRandomDirectionInCone(m_weaponDef->m_projectileCone) + offset, m_weaponDef->m_projectileSpeed);
	}
	for (int i = 0; i < m_weaponDef->m_meleeCount; i++)
	{
		for (int actorIndex = 0; actorIndex < (int)g_currentMap->m_allActorsList.size(); actorIndex++)
		{
			if (!g_currentMap->m_allActorsList[actorIndex])
			{
				continue;
			}
			ActorUID targetUID = g_currentMap->m_allActorsList[actorIndex]->m_UID;
			if (m_holder == targetUID || !targetUID->m_actorDef->m_visible || m_holder->m_actorDef->m_faction == targetUID->m_actorDef->m_faction)
			{
				continue;
			}

			Vec2 targetXY = Vec2(targetUID->m_position.x, targetUID->m_position.y);
			Vec2 holderXY = Vec2(m_holder->m_position.x, m_holder->m_position.y);
			Vec2 holderForward = Vec2(m_holder->GetActorForward().x, m_holder->GetActorForward().y).GetNormalized();
			if (IsPointInsideDirectedSector2D(targetXY, holderXY, holderForward, m_weaponDef->m_meleeArc, m_weaponDef->m_meleeRange))
			{
				int damage = g_theRNG->RollRandomIntInRange((int)m_weaponDef->m_meleeDamage.m_min, (int)m_weaponDef->m_meleeDamage.m_max);
				targetUID->TakeDamage(damage, m_holder);
				targetUID->AddImpulse(m_holder->GetActorForward(), m_weaponDef->m_meleeImpulse);
			}
		}
	}
	m_refireTimer = m_weaponDef->m_refireTime;
}

Vec3 Weapon::GetRandomDirectionInCone(float coneRadius)
{
	EulerAngles result;
	if (m_holder->m_controller->IsPlayer())
	{
		result = reinterpret_cast<PlayerController*>(m_holder->m_controller)->m_cameraOrientation;
	}
	else
	{
		result = m_holder->m_orientationDegrees;
	}
	float xAngle = g_theRNG->RollRandomFloatInRange(0.f, 360.f);
	float yAngle = g_theRNG->RollRandomFloatInRange(0.f, 360.f);

	float x = SinDegrees(xAngle) * CosDegrees(xAngle);
	float y = SinDegrees(yAngle) * SinDegrees(yAngle);

	result += EulerAngles(x * coneRadius, y * coneRadius, 0);
	return result.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
}

void Weapon::Reload()
{
	if (m_currentMagCount >= m_weaponDef->m_magazine)
	{
		return;
	}
	if (!m_weaponDef->m_overrideCurrentSound)
	{
		g_theAudio->StopSound(m_currentSound.m_soundPlaybackID);
	}

	m_reloadingTimer->Start();
	m_isReloading = true;
}

void Weapon::PullBackFishingRod()
{
	if (!m_bait.IsValid())
	{
		return;
	}
	if (m_fishingPullTimer->IsStopped())
	{
		m_fishingThrowTimer->Stop();
		m_fishingPullTimer->Start();

		m_bait->m_isStatic = false;
		m_bait->m_isFlying = true;
		Vec3 toHolder = m_holder->m_position - m_bait->m_position;
		toHolder -= m_holder->GetModelMatrix().GetJBasis3D() * 0.5f;
		toHolder.GetNormalized();
		toHolder.z = 3.5f;
		m_bait->AddImpulse(toHolder, 3.5f);

		PlaySoundByName("Fire", m_weaponDef->m_overrideCurrentSound);
	}
}

void Weapon::AddBulletFromTotalToMag()
{
	if (m_currentAmmoCount < m_weaponDef->m_magazine)
	{
		m_currentMagCount += m_currentAmmoCount;
		m_currentAmmoCount = 0;
	}
	else
	{
		int bulletToReload = m_weaponDef->m_magazine - m_currentMagCount;
		m_currentMagCount += bulletToReload;
		m_currentAmmoCount -= bulletToReload;
	}
}

void Weapon::PlayAnimationByName(std::string name)
{
	bool hasAnim = false;
	if (m_currentAnimation.m_name == name)
	{
		if (m_currentAnimation.m_playbackMode == "Once" && m_animationClock->GetTotalSeconds() < m_currentAnimation.m_duration)
		{
			return;
		}
	}
	for (size_t i = 0; i < m_weaponDef->m_animation.size(); i++)
	{
		if (m_weaponDef->m_animation[i].m_name == name)
		{
			m_currentAnimation = m_weaponDef->m_animation[i];
			hasAnim = true;
			break;
		}
	}
	if (!hasAnim && m_weaponDef->m_animation.size() > 0)
	{
		m_currentAnimation = m_weaponDef->m_animation[0];
	}
	if (m_currentAnimation.m_playbackMode == "Once")
	{
		m_animationClock->Reset();
	}
}

void Weapon::PlaySoundByName(std::string name, bool overrideTheCurrentSound)
{
	if (!overrideTheCurrentSound)
	{
		if (m_currentSound.m_name == name && g_theAudio->IsPlaying(m_currentSound.m_soundPlaybackID))
		{
			return;
		}
		else
		{
			g_theAudio->StopSound(m_currentSound.m_soundPlaybackID);
		}
	}

	for (size_t i = 0; i < m_soundsList.size(); i++)
	{
		if (m_soundsList[i].m_name == name)
		{
			m_currentSound.m_name = name;
			m_currentSound.m_soundPlaybackID = g_theAudio->StartSoundAt(m_soundsList[i].m_soundID, m_holder->m_position, false, g_gameConfigBlackboard.GetValue("sfxVolume", 1.0f));
			break;
		}
	}
}

bool Weapon::HasAmmo() const
{
	if (m_weaponDef->m_meleeCount >= 0)
	{
		return true;
	}
	return (m_weaponDef->m_magazine == -1) ? true : m_currentAmmoCount > 0 || m_currentMagCount > 0;
}

void Weapon::StopAllFishingTimer()
{
	m_fishingThrowTimer->Stop();
	m_fishingPullTimer->Stop();
}
