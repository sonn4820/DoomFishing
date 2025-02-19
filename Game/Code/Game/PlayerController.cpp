#include "PlayerController.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Weapon.hpp"


PlayerController::~PlayerController()
{
	m_playerCamera = nullptr;
}

bool PlayerController::IsPlayer() const
{
	return true;
}

void PlayerController::Possess(ActorUID actorUID)
{
	Controller::Possess(actorUID);
	m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, actorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
	m_eyeHeight = actorUID->m_actorDef->m_eyeHeight;
	if (!m_isFreefly)
	{
		m_cameraOrientation = m_controllerActorUID->m_orientationDegrees;
		m_cameraPosition = Vec3(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y, m_controllerActorUID->m_position.z + m_eyeHeight);
	}
	m_playerCamera->SetTransform(m_cameraPosition, m_cameraOrientation);
}

void PlayerController::Initialize(AABB2 cameraBox)
{
	m_playerCamera = new Camera();
	m_playerScreenCamera = new Camera();
	m_playerCamera->SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_playerCamera->SetViewportUVs(cameraBox);


	Vec2 BL = Vec2::ZERO + Vec2(0.f, Window::GetMainWindowInstance()->GetClientDimensions().y * cameraBox.m_mins.y);
	Vec2 TR = Vec2::ZERO + Vec2((float)Window::GetMainWindowInstance()->GetClientDimensions().x, Window::GetMainWindowInstance()->GetClientDimensions().y * cameraBox.m_maxs.y);
	m_playerScreenCamera->SetOrthographicView(BL, TR);
	m_playerScreenCamera->SetViewportUVs(cameraBox);

	m_flashRedTimer = new Timer(0.5f, g_theGame->m_clock);
}

void PlayerController::Update()
{
	UpdateInput();
	InteractionWithWorld();
	if (m_flashRedTimer->HasPeriodElapsed())
	{
		m_flashRedTimer->Stop();
	}
}

Camera* PlayerController::GetCamera() const
{
	return m_playerCamera;
}

Camera* PlayerController::GetScreenCamera() const
{
	return m_playerScreenCamera;
}

Mat44 PlayerController::GetModelMatrix() const
{
	Mat44 modelMat = Mat44();
	modelMat = m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelMat.SetTranslation3D(m_cameraPosition);
	return modelMat;
}

void PlayerController::SetCameraWhenSwitch()
{
	if (m_isFreefly)
	{
		m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect(), 60.f, 0.1f, 100.f);
		Vec3 cameraPos;
		if (GetActor())
		{
			cameraPos = m_controllerActorUID->m_position;
		}
		else
		{
			cameraPos = m_cameraPosition;
		}

		cameraPos.z = cameraPos.z + m_controllerActorUID->m_actorDef->m_eyeHeight;
		m_cameraPosition = cameraPos;
	}
	else
	{
		if (GetActor())
		{
			m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect(), m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f);
		}
		m_cameraOrientation.m_pitchDegrees = m_recordedActorPitch;
		m_cameraOrientation.m_yawDegrees = m_recordedActorYaw;
	}
	m_playerCamera->SetTransform(m_cameraPosition, m_cameraOrientation);
}

SpriteAnimDefinition PlayerController::GetCurrentWeaponAnim() const
{
	int startFrame = GetActor()->m_currentWeapon->m_currentAnimation.m_startFrame;
	int endFrame = GetActor()->m_currentWeapon->m_currentAnimation.m_endFrame;
	float duration = GetActor()->m_currentWeapon->m_currentAnimation.m_duration;
	SpriteSheet* spriteSheet = GetActor()->m_currentWeapon->m_weaponDef->m_weaponSpriteSheet;
	SpriteAnimPlaybackType type = SpriteAnimPlaybackType::ONCE;
	if (GetActor()->m_currentWeapon->m_currentAnimation.m_playbackMode == "PingPong")
	{
		type = SpriteAnimPlaybackType::PINGPONG;
	}
	SpriteAnimDefinition animSprite = SpriteAnimDefinition(*spriteSheet, startFrame, endFrame, duration, type);

	return animSprite;
}

AABB2 PlayerController::GetTextPositionOnHUD(float valueZeroToOneOnX, float valueZeroToOneOnY, float cellHeight, float cellAspect) const
{
	Vec2 topRightHUD = Vec2(GetScreenCamera()->GetOrthographicTopRight().x, GetScreenCamera()->GetOrthographicBottomLeft().y + (GetScreenCamera()->GetOrthographicTopRight().y - GetScreenCamera()->GetOrthographicBottomLeft().y) * 0.15f);
	Vec2 bottomLeft = Vec2(GetScreenCamera()->GetOrthographicBottomLeft().x + (GetScreenCamera()->GetOrthographicTopRight().x - GetScreenCamera()->GetOrthographicBottomLeft().x) * valueZeroToOneOnX,
		GetScreenCamera()->GetOrthographicBottomLeft().y + (topRightHUD.y - GetScreenCamera()->GetOrthographicBottomLeft().y) * valueZeroToOneOnY);
	Vec2 topRight = bottomLeft + Vec2(cellHeight * cellAspect * (float)g_theGame->m_numPlayer, cellHeight) / (float)g_theGame->m_numPlayer;
	return AABB2(bottomLeft, topRight);
}

Vec2 PlayerController::GetTextMinPositionOnScreen(float valueZeroToOneOnX, float valueZeroToOneOnY) const
{
	return Vec2(GetScreenCamera()->GetOrthographicBottomLeft().x + (GetScreenCamera()->GetOrthographicTopRight().x - GetScreenCamera()->GetOrthographicBottomLeft().x) * valueZeroToOneOnX,
		GetScreenCamera()->GetOrthographicBottomLeft().y + (GetScreenCamera()->GetOrthographicTopRight().y - GetScreenCamera()->GetOrthographicBottomLeft().y) * valueZeroToOneOnY);
}

void PlayerController::UpdateInput()
{
	if (g_theGame->m_numPlayer == 1)
	{
		if (g_theInput->WasKeyJustPressed('X'))
		{
			if (GetActor())
			{
				m_isFreefly = !m_isFreefly;
				SetCameraWhenSwitch();
				m_controllerActorUID->PlayAnimationByName("Default");
			}
		}
	}

	if (m_isFreefly)
	{
		UpdateFreeFlyMode();
	}
	else
	{
		UpdateProcessedMode();
	}
}

void PlayerController::UpdateCamera()
{
	if (!GetActor())
	{
		return;
	}
	if (m_controllerActorUID->m_isDead)
	{
		m_eyeHeight += (0.f - m_eyeHeight) / m_controllerActorUID->m_actorDef->m_corpseLifetime * g_theGame->m_clock->GetDeltaSeconds();
		m_cameraPosition = Vec3(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y, m_eyeHeight);
		m_playerCamera->SetTransform(m_cameraPosition, m_cameraOrientation);
		return;
	}

	if (!m_isFreefly)
	{
		m_cameraOrientation.m_pitchDegrees = Clamp(m_cameraOrientation.m_pitchDegrees, -85.f, 85.f);
		m_controllerActorUID->m_orientationDegrees.m_yawDegrees = m_cameraOrientation.m_yawDegrees;
		m_cameraPosition = Vec3(m_controllerActorUID->m_position.x, m_controllerActorUID->m_position.y, m_controllerActorUID->m_position.z + m_controllerActorUID->m_actorDef->m_eyeHeight);
	}

	m_playerCamera->SetTransform(m_cameraPosition, m_cameraOrientation);
}

void PlayerController::UpdateProcessedMode()
{
	if (!GetActor() || m_controllerActorUID->m_isDead)
	{
		return;
	}

	if (m_controllerIndex == -1)
	{
		UpdateMKBInput();
	}
	else
	{
		UpdateControllerInput(m_controllerIndex);
	}
}

void PlayerController::UpdateControllerInput(int controllerID)
{
	Vec3 forwardDir = GetModelMatrix().GetIBasis3D().GetNormalized();
	Vec3 rightDir = GetModelMatrix().GetJBasis3D().GetNormalized();
	Vec3 upDir = GetModelMatrix().GetKBasis3D().GetNormalized();

	Vec3 currentDir;
	float currentSpeed = 0.f;

	if (g_theInput->GetController(controllerID).GetLeftStick().GetMagnitude() > 0)
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_walkSpeed;
		float leftStickPosX = g_theInput->GetController(controllerID).GetLeftStick().GetPosition().x;
		float leftStickPosY = g_theInput->GetController(controllerID).GetLeftStick().GetPosition().y;
		currentDir += Vec3(forwardDir.x, forwardDir.y, 0).GetNormalized() * leftStickPosY;
		currentDir -= Vec3(rightDir.x, rightDir.y, 0).GetNormalized() * leftStickPosX;
	}
	if (g_theInput->GetController(controllerID).GetRightStick().GetMagnitude() > 0)
	{
		float rightStickPosX = g_theInput->GetController(controllerID).GetRightStick().GetPosition().x;
		float rightStickPosY = g_theInput->GetController(controllerID).GetRightStick().GetPosition().y;
		m_cameraOrientation.m_pitchDegrees -= rightStickPosY * m_controllerActorUID->m_actorDef->m_turnSpeed * g_theGame->m_clock->GetDeltaSeconds();
		m_cameraOrientation.m_yawDegrees -= rightStickPosX * m_controllerActorUID->m_actorDef->m_turnSpeed * g_theGame->m_clock->GetDeltaSeconds();
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_SHOULDER_RIGHT))
	{
		if (m_controllerActorUID->m_currentWeapon->m_isFishingRod)
		{
			if (m_controllerActorUID->m_currentWeapon->m_isCurrentlyFishing)
			{
				m_controllerActorUID->m_currentWeapon->PullBackFishingRod();
			}
			else
			{
				m_controllerActorUID->Attack();
			}
		}
	}
	if (g_theInput->GetController(controllerID).GetRightTrigger() > 0.f)
	{
		if (!m_controllerActorUID->m_currentWeapon->m_isFishingRod)
		{
			if (m_controllerActorUID->m_currentWeapon->HasAmmo() && !m_controllerActorUID->m_currentWeapon->m_isReloading)
			{
				m_controllerActorUID->Attack();
				if (m_controllerActorUID->m_currentWeapon->m_weaponDef->m_canScope)
				{
					m_controllerActorUID->m_currentWeapon->m_isScoping = false;
					m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
				}
			}
		}
	}
	else
	{
		if (m_controllerActorUID->m_currentWeapon != nullptr && !m_controllerActorUID->m_currentWeapon->m_soundOverrided)
		{
			g_theAudio->StopSound(m_controllerActorUID->m_currentWeapon->m_currentSound.m_soundPlaybackID);
		}
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_SHOULDER_LEFT))
	{
		if (m_controllerActorUID->m_currentWeapon->m_weaponDef->m_canScope && m_controllerActorUID->m_currentWeapon->m_refireTimer < 0.f)
		{
			m_controllerActorUID->m_currentWeapon->m_isScoping = !m_controllerActorUID->m_currentWeapon->m_isScoping;

			if (m_controllerActorUID->m_currentWeapon->m_isScoping)
			{
				m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, 10.f, 0.1f, 100.f, false);
			}
			else
			{
				m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
			}
		}
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_X))
	{
		m_controllerActorUID->EquipWeapon(0);
		m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_Y))
	{
		m_controllerActorUID->EquipWeapon(1);
		m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_A))
	{
		Interact();
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_DPAD_UP))
	{
		if (m_controllerActorUID->m_currentWeaponIndex > 0)
		{
			--m_controllerActorUID->m_currentWeaponIndex;
		}
		else
		{
			m_controllerActorUID->m_currentWeaponIndex = (int)m_controllerActorUID->m_weaponInventory.size() - 1;
		}

		m_controllerActorUID->EquipWeapon(m_controllerActorUID->m_currentWeaponIndex);
	}
	if (g_theInput->GetController(controllerID).WasButtonJustPressed(XBOX_BUTTON_DPAD_DOWN))
	{
		if (m_controllerActorUID->m_currentWeaponIndex < (int)m_controllerActorUID->m_weaponInventory.size() - 1)
		{
			++m_controllerActorUID->m_currentWeaponIndex;
		}
		else
		{
			m_controllerActorUID->m_currentWeaponIndex = 0;
		}

		m_controllerActorUID->EquipWeapon(m_controllerActorUID->m_currentWeaponIndex);
	}
	if (m_controllerActorUID->m_isFlying)
	{
		if (g_theInput->GetController(controllerID).IsButtonDown(XBOX_BUTTON_SHOULDER_LEFT))
		{
			currentDir += Vec3(0.f, 0.f, 1.f);
		}
		if (g_theInput->GetController(controllerID).IsButtonDown(XBOX_BUTTON_SHOULDER_RIGHT))
		{
			currentDir -= Vec3(0.f, 0.f, 1.f);
		}
	}

	if (g_theInput->GetController(controllerID).GetLeftTrigger() > 0.f)
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_runSpeed;
	}
	if (g_theInput->GetController(controllerID).IsButtonDown(XBOX_BUTTON_B))
	{
		if (m_controllerActorUID->m_currentWeapon->m_currentAmmoCount <= 0)
		{
			return;
		}
		m_controllerActorUID->m_currentWeapon->Reload();
	}

	m_controllerActorUID->MoveInDirection(currentDir, currentSpeed);
	m_recordedActorPitch = m_cameraOrientation.m_pitchDegrees;
	m_recordedActorYaw = m_cameraOrientation.m_yawDegrees;
}

void PlayerController::UpdateMKBInput()
{
	Vec3 forwardDir = GetModelMatrix().GetIBasis3D().GetNormalized();
	Vec3 rightDir = GetModelMatrix().GetJBasis3D().GetNormalized();
	Vec3 upDir = GetModelMatrix().GetKBasis3D().GetNormalized();

	Vec3 currentDir;
	float currentSpeed = 0.f;

	if (g_theGame->m_numPlayer == 1)
	{
		if (g_theInput->WasKeyJustPressed('N'))
		{
			//g_currentMap->DebugPossessNext();
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		if (m_controllerActorUID->m_currentWeapon->m_isFishingRod)
		{
			if (m_controllerActorUID->m_currentWeapon->m_isCurrentlyFishing)
			{
				m_controllerActorUID->m_currentWeapon->PullBackFishingRod();
			}
			else
			{
				m_controllerActorUID->Attack();
			}
		}
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		if (!m_controllerActorUID->m_currentWeapon->m_isFishingRod)
		{
			if (m_controllerActorUID->m_currentWeapon->HasAmmo() && !m_controllerActorUID->m_currentWeapon->m_isReloading)
			{
				m_controllerActorUID->Attack();
				if (m_controllerActorUID->m_currentWeapon->m_weaponDef->m_canScope)
				{
					m_controllerActorUID->m_currentWeapon->m_isScoping = false;
					m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
				}
			}
		}
	}
	else
	{
		if (m_controllerActorUID->m_currentWeapon != nullptr && !m_controllerActorUID->m_currentWeapon->m_soundOverrided)
		{
			g_theAudio->StopSound(m_controllerActorUID->m_currentWeapon->m_currentSound.m_soundPlaybackID);
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		if (m_controllerActorUID->m_currentWeapon->m_weaponDef->m_canScope && m_controllerActorUID->m_currentWeapon->m_refireTimer < 0.f)
		{
			m_controllerActorUID->m_currentWeapon->m_isScoping = !m_controllerActorUID->m_currentWeapon->m_isScoping;

			if (m_controllerActorUID->m_currentWeapon->m_isScoping)
			{
				m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, 10.f, 0.1f, 100.f, false);
			}
			else
			{
				m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
			}
		}
	}
	if (g_theInput->WasKeyJustPressed(49))
	{
		m_controllerActorUID->EquipWeapon(0);
		m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
	}
	if (g_theInput->WasKeyJustPressed(50))
	{
		m_controllerActorUID->EquipWeapon((int)m_controllerActorUID->m_weaponInventory.size() - 1);
		m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
	}
	if (g_theInput->WasKeyJustPressed('R'))
	{
		if (m_controllerActorUID->m_currentWeapon->m_currentAmmoCount <= 0)
		{
			return;
		}
		m_controllerActorUID->m_currentWeapon->Reload();
	}
	if (g_theInput->WasKeyJustPressed('Q'))
	{
		if (m_controllerActorUID->m_currentWeaponIndex > 0)
		{
			--m_controllerActorUID->m_currentWeaponIndex;
		}
		else
		{
			m_controllerActorUID->m_currentWeaponIndex = (int)m_controllerActorUID->m_weaponInventory.size() - 1;
		}

		m_controllerActorUID->EquipWeapon(m_controllerActorUID->m_currentWeaponIndex);
	}
	if (g_theInput->WasKeyJustPressed('E'))
	{
		if (m_controllerActorUID->m_currentWeaponIndex < (int)m_controllerActorUID->m_weaponInventory.size() - 1)
		{
			++m_controllerActorUID->m_currentWeaponIndex;
		}
		else
		{
			m_controllerActorUID->m_currentWeaponIndex = 0;
		}

		m_controllerActorUID->EquipWeapon(m_controllerActorUID->m_currentWeaponIndex);
	}

	float currentMouseRate = 0.075f;
	if (m_controllerActorUID->m_currentWeapon != nullptr && m_controllerActorUID->m_currentWeapon->m_isScoping)
	{
		currentMouseRate *= 0.5f;
	}

	m_cameraOrientation.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * currentMouseRate;
	m_cameraOrientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * currentMouseRate;

	if (g_theInput->WasKeyJustPressed('F'))
	{
		Interact();
	}

	if (g_theInput->IsKeyDown('W'))
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_walkSpeed;
		currentDir += Vec3(forwardDir.x, forwardDir.y, 0).GetNormalized();
	}
	if (g_theInput->IsKeyDown('S'))
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_walkSpeed;
		currentDir += Vec3(forwardDir.x, forwardDir.y, 0).GetNormalized() * -1.f;
	}
	if (g_theInput->IsKeyDown('A'))
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_walkSpeed;
		currentDir += Vec3(rightDir.x, rightDir.y, 0).GetNormalized();
	}
	if (g_theInput->IsKeyDown('D'))
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_walkSpeed;
		currentDir += Vec3(rightDir.x, rightDir.y, 0).GetNormalized() * -1.f;
	}
	if (m_controllerActorUID->m_isFlying)
	{
		if (g_theInput->IsKeyDown(KEYCODE_SPACE))
		{
			currentDir += Vec3(0.f, 0.f, 1.f);
		}
		if (g_theInput->IsKeyDown(KEYCODE_CTRL))
		{
			currentDir -= Vec3(0.f, 0.f, 1.f);
		}
	}
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		currentSpeed = m_controllerActorUID->m_actorDef->m_runSpeed;
	}
	float extraMoveSpeed = (m_controllerActorUID->m_currentWeapon != nullptr) ? m_controllerActorUID->m_currentWeapon->m_weaponDef->m_moveSpeed : 1.f;
	m_controllerActorUID->MoveInDirection(currentDir, currentSpeed * extraMoveSpeed);
	m_recordedActorPitch = m_cameraOrientation.m_pitchDegrees;
	m_recordedActorYaw = m_cameraOrientation.m_yawDegrees;
}

void PlayerController::DamagedBy(ActorUID dealer)
{
	if (!m_controllerActorUID->m_isDead)
	{
		m_controllerActorUID->PlayAnimationByName("Hurt");
		if (g_theRNG->RollRandomChance(0.5f))
		{
			m_controllerActorUID->PlaySoundByName("Hurt1", false);
		}
		else
		{
			if (g_theRNG->RollRandomChance(0.5f))
			{
				m_controllerActorUID->PlaySoundByName("Hurt2", false);
			}
			else
			{
				m_controllerActorUID->PlaySoundByName("Hurt3", false);
			}
		}
	}
	else
	{
		Reset();
	}
	if (dealer->m_controller->IsPlayer())
	{
		if (m_controllerActorUID->m_isDead)
		{
			reinterpret_cast<PlayerController*>(dealer->m_controller)->m_numPlayerKilled++;
			reinterpret_cast<PlayerController*>(dealer->m_controller)->m_killStreak++;
			int killStreak = reinterpret_cast<PlayerController*>(dealer->m_controller)->m_killStreak;
			if (killStreak == 2)
			{
				dealer->PlaySoundByName("DoubleK");
			}
			if (killStreak == 3)
			{
				dealer->PlaySoundByName("TripleK");
			}
			if (killStreak == 4)
			{
				dealer->PlaySoundByName("QuadraK");
			}
			if (killStreak == 5)
			{
				dealer->PlaySoundByName("PentaK");
			}
			if (killStreak >= 6)
			{
				dealer->PlaySoundByName("LegendK");
			}
			m_numPlayerDeath++;
			m_killStreak = 0;
		}
	}

	m_flashRedTimer->Start();
}

void PlayerController::RenderHUD() const
{
	if (m_isFreefly)
	{
		return;
	}

	Vec2 topRightHUD = Vec2(GetScreenCamera()->GetOrthographicTopRight().x, GetScreenCamera()->GetOrthographicBottomLeft().y + (GetScreenCamera()->GetOrthographicTopRight().y - GetScreenCamera()->GetOrthographicBottomLeft().y) * 0.15f);
	AABB2 nonHUDScreenBound = AABB2(Vec2(GetScreenCamera()->GetOrthographicBottomLeft().x, topRightHUD.y), GetScreenCamera()->GetOrthographicTopRight());
	AABB2 fullScreenBound = AABB2(GetScreenCamera()->GetOrthographicBottomLeft(), GetScreenCamera()->GetOrthographicTopRight());

	if (GetActor()->m_currentWeapon)
	{
		WeaponDefinition* weaponDef = GetActor()->m_currentWeapon->m_weaponDef;

		g_theRenderer->BeginCamera(*GetScreenCamera());

		g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);
		g_theRenderer->BindShader(weaponDef->m_shader);
		g_theRenderer->SetModelConstants();

		if (!GetActor()->m_isDead)
		{
			std::vector<Vertex_PCU> reticle;
			Vec2 center = Vec2(GetScreenCamera()->GetOrthographicTopRight().x * 0.5f, GetScreenCamera()->GetOrthographicBottomLeft().y + (GetScreenCamera()->GetOrthographicTopRight().y - GetScreenCamera()->GetOrthographicBottomLeft().y) * 0.5f);
			IntVec2 reticleHalfSize = weaponDef->m_reticleSize / 2;
			AABB2 reticleBound = AABB2(center - Vec2((float)reticleHalfSize.x, (float)reticleHalfSize.y), center + Vec2((float)reticleHalfSize.x, (float)reticleHalfSize.y));
			AddVertsForAABB2D(reticle, reticleBound, Rgba8::COLOR_WHITE);
			g_theRenderer->BindTexture(weaponDef->m_reticleTexture);
			g_theRenderer->DrawVertexArray((int)reticle.size(), reticle.data());

			if (m_controllerActorUID->m_currentWeapon->m_isScoping)
			{
				g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
				g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);
				g_theRenderer->BindShader(nullptr);
				g_theRenderer->SetModelConstants();

				std::vector<Vertex_PCU> scopeScreen;
				AddVertsForAABB2D(scopeScreen, nonHUDScreenBound, Rgba8::COLOR_WHITE);

				g_theRenderer->BindTexture(m_controllerActorUID->m_currentWeapon->m_weaponDef->m_scopeTexture);
				g_theRenderer->DrawVertexArray((int)scopeScreen.size(), scopeScreen.data());
			}
			else
			{
				if (!m_controllerActorUID->m_currentWeapon->m_isFishingRod)
				{
					std::vector<Vertex_PCU> weapon;
					float weaponWidth = weaponDef->m_spriteSize.x / (float)g_theGame->m_numPlayer;
					float weaponHeight = weaponDef->m_spriteSize.y / (float)g_theGame->m_numPlayer;
					Vec2 alignment = Vec2(weaponWidth * weaponDef->m_spritePivot.x, weaponHeight * weaponDef->m_spritePivot.y);
					Vec2 BL = Vec2(GetScreenCamera()->GetOrthographicTopRight().x * 0.5f, topRightHUD.y) - alignment;
					Vec2 TR = Vec2(GetScreenCamera()->GetOrthographicTopRight().x * 0.5f + weaponWidth, topRightHUD.y + weaponHeight) - alignment;
					AABB2 weaponBound(BL, TR);
					SpriteAnimDefinition weaponAnim = GetCurrentWeaponAnim();
					SpriteDefinition weaponSprite = weaponAnim.GetSpriteDefAtTime(GetActor()->m_currentWeapon->m_animationClock->GetTotalSeconds());
					AABB2 animSpriteUV = weaponSprite.GetUVs();
					AddVertsForAABB2D(weapon, weaponBound, Rgba8::COLOR_WHITE, animSpriteUV.m_mins, animSpriteUV.m_maxs);
					g_theRenderer->BindTexture(&weaponDef->m_weaponSpriteSheet->GetTexture());
					g_theRenderer->DrawVertexArray((int)weapon.size(), weapon.data());
				}
			}
		}

		std::vector<Vertex_PCU> hud;
		AABB2 hudBound = AABB2(GetScreenCamera()->GetOrthographicBottomLeft(), topRightHUD);
		AddVertsForAABB2D(hud, hudBound, Rgba8::COLOR_WHITE);
		g_theRenderer->BindTexture(weaponDef->m_baseTexture);
		g_theRenderer->DrawVertexArray((int)hud.size(), hud.data());

		std::vector<Vertex_PCU> textHUD;
		AABB2 textKillPosistion = GetTextPositionOnHUD(0.05f, 0.4f, 50.f);
		AABB2 textAmmoPosistion = GetTextPositionOnHUD(0.15f, 0.22f, 100.f);
		AABB2 textHPPosistion = GetTextPositionOnHUD(0.255f, 0.42f, 50.f, 3.f);
		AABB2 textDeathPosistion = GetTextPositionOnHUD(0.92f, 0.4f, 50.f);
		AABB2 textMoneyPosistion = GetTextPositionOnHUD(0.57f, 0.15f, 100.f);
		g_theFont->AddVertsForTextInBox2D(textHUD, textKillPosistion, 50.f, Stringf("%i", m_numPlayerKilled));
		if (m_controllerActorUID->m_currentWeapon->m_weaponDef->m_magazine == -1)
		{
			g_theFont->AddVertsForTextInBox2D(textHUD, textAmmoPosistion, 50.f, " inf");
		}
		else
		{
			g_theFont->AddVertsForTextInBox2D(textHUD, textAmmoPosistion, 50.f, Stringf("%i/%i", m_controllerActorUID->m_currentWeapon->m_currentMagCount, m_controllerActorUID->m_currentWeapon->m_currentAmmoCount));
		}
		g_theFont->AddVertsForTextInBox2D(textHUD, textHPPosistion, 50.f, Stringf("%i", GetActor()->m_currentHealth));
		g_theFont->AddVertsForTextInBox2D(textHUD, textDeathPosistion, 50.f, Stringf("%i", m_numPlayerDeath));
		g_theFont->AddVertsForTextInBox2D(textHUD, textMoneyPosistion, 50.f, Stringf("$%i", m_money));
		if (!m_controllerActorUID->m_currentWeapon->HasAmmo())
		{
			Vec2 warningText = GetTextMinPositionOnScreen(0.46f, 0.6f);
			g_theFont->AddVertsForText2D(textHUD, warningText, 20.f, "NO AMMO", Rgba8::COLOR_RED);
		}
		if (m_controllerActorUID->m_currentWeapon->m_isReloading)
		{
			Vec2 reloadingText = GetTextMinPositionOnScreen(0.45f, 0.6f);
			g_theFont->AddVertsForText2D(textHUD, reloadingText, 20.f, "RELOADING", Rgba8::COLOR_YELLOW);
		}
 		if (m_currentInteraction.m_didImpact && m_currentInteraction.m_targetUID.IsValid())
		{
			if (m_currentInteraction.m_targetUID->m_actorDef->m_faction == "Station")
			{
				Vec2 buyText = GetTextMinPositionOnScreen(0.4f, 0.4f);

				if (m_controllerIndex == -1)
				{
					if (m_controllerActorUID->HasWeapon(m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_name))
					{
						g_theFont->AddVertsForText2D(textHUD, buyText, 20.f, Stringf("Press F To Buy Ammo\n\nPrice: $%i", m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_magPrice), Rgba8::COLOR_YELLOW);
					}
					else
					{
						g_theFont->AddVertsForText2D(textHUD, buyText, 20.f, Stringf("Press F To Buy Gun\n\nPrice: $%i", m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_price), Rgba8::COLOR_YELLOW);
					}
				}

				if (m_controllerIndex >= 0)
				{
					if (m_controllerActorUID->HasWeapon(m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_name))
					{
						g_theFont->AddVertsForText2D(textHUD, buyText, 20.f, Stringf("Press A To Buy Ammo\n\nPrice: $%i", m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_magPrice), Rgba8::COLOR_YELLOW);
					}
					else
					{
						g_theFont->AddVertsForText2D(textHUD, buyText, 20.f, Stringf("Press A To Buy Gun\n\nPrice: $%i", m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0]->m_price), Rgba8::COLOR_YELLOW);
					}
				}

			}

		}
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray((int)textHUD.size(), textHUD.data());



		g_theRenderer->EndCamera(*GetScreenCamera());
	}

	if (GetActor()->m_isDead)
	{
		g_theRenderer->BeginCamera(*GetScreenCamera());

		g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetModelConstants();

		std::vector<Vertex_PCU> deadScreen;
		float alpha = Interpolate(240, 150, GetActor()->m_corpseTimer);
		AddVertsForAABB2D(deadScreen, fullScreenBound, Rgba8(0, 0, 0, (unsigned char)alpha));

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)deadScreen.size(), deadScreen.data());
		g_theRenderer->EndCamera(*GetScreenCamera());
	}
	else
	{
		if (!m_flashRedTimer->IsStopped())
		{
			g_theRenderer->BeginCamera(*GetScreenCamera());
			g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
			g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);
			g_theRenderer->BindShader(nullptr);
			g_theRenderer->SetModelConstants();

			std::vector<Vertex_PCU> flashRedScreen;
			float alpha = Interpolate(70, 0, SmoothStart2(m_flashRedTimer->GetElapsedFraction()));
			AddVertsForAABB2D(flashRedScreen, nonHUDScreenBound, Rgba8(255, 0, 0, (unsigned char)alpha));

			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray((int)flashRedScreen.size(), flashRedScreen.data());

			g_theRenderer->EndCamera(*GetScreenCamera());
		}
	}
}

void PlayerController::InteractionWithWorld()
{
	Mat44 cameraMat = m_playerCamera->GetModelMatrix();
	m_currentInteraction = g_currentMap->RaycastActors(m_cameraPosition, cameraMat.GetIBasis3D(), 1.f, m_controllerActorUID);
	if (m_currentInteraction.m_didImpact && m_currentInteraction.m_targetUID->m_actorDef->m_name == "Bait")
	{
		m_currentInteraction.m_didImpact = false;
	}
}

void PlayerController::Interact()
{
	if (m_currentInteraction.m_didImpact && m_currentInteraction.m_targetUID->m_actorDef->m_faction == "Station")
	{
		WeaponDefinition* stationWeapon = m_currentInteraction.m_targetUID->m_actorDef->m_inventory[0];
		if (m_controllerActorUID->HasWeapon(stationWeapon->m_name))
		{
			if (m_money > stationWeapon->m_magPrice)
			{
				m_money -= stationWeapon->m_magPrice;
				m_controllerActorUID->HasWeapon(stationWeapon->m_name)->m_currentAmmoCount += stationWeapon->m_magazine;
				m_controllerActorUID->PlaySoundByName("Buy", true);
			}
			else
			{
				m_controllerActorUID->PlaySoundByName("NoMoney", true);
			}
		}
		else
		{
			if (m_money > stationWeapon->m_price)
			{
				m_money -= stationWeapon->m_price;
				m_controllerActorUID->m_weaponInventory.push_back(new Weapon(stationWeapon));
				m_controllerActorUID->EquipWeapon((int)m_controllerActorUID->m_weaponInventory.size() - 1);
				m_playerCamera->SetPerspectiveView(Window::GetMainWindowInstance()->GetAspect() * g_theGame->m_numPlayer, m_controllerActorUID->m_actorDef->m_cameraFOV, 0.1f, 100.f, false);
				m_controllerActorUID->PlaySoundByName("Buy", true);
			}
			else
			{
				m_controllerActorUID->PlaySoundByName("NoMoney", true);
			}
		}
	}
}

void PlayerController::AddMoney(int amount)
{
	m_controllerActorUID->PlaySoundByName("Money");
	m_money += amount;
}

void PlayerController::Reset()
{
	m_money = 0;
}

void PlayerController::UpdateFreeFlyMode()
{
	if (!GetActor() || m_controllerActorUID->m_isDead)
	{
		return;
	}
	Vec3 forwardDir = GetModelMatrix().GetIBasis3D().GetNormalized();
	Vec3 rightDir = GetModelMatrix().GetJBasis3D().GetNormalized();
	Vec3 upDir = GetModelMatrix().GetKBasis3D().GetNormalized();

	float walk = 1.f;
	float run = 15.f;
	float rotateSpeed = 90.f;
	Vec3 velocity = Vec3::ZERO;

	// CONTROLLER
	if (g_theInput->GetController(0).GetLeftStick().GetMagnitude() > 0)
	{
		float leftStickPosX = g_theInput->GetController(0).GetLeftStick().GetPosition().x;
		float leftStickPosY = g_theInput->GetController(0).GetLeftStick().GetPosition().y;
		velocity += forwardDir * leftStickPosY * walk;
		velocity -= rightDir * leftStickPosX * walk;
	}
	if (g_theInput->GetController(0).GetRightStick().GetMagnitude() > 0)
	{
		float rightStickPosX = g_theInput->GetController(0).GetRightStick().GetPosition().x;
		float rightStickPosY = g_theInput->GetController(0).GetRightStick().GetPosition().y;
		m_cameraOrientation.m_pitchDegrees -= rightStickPosY * rotateSpeed * g_theGame->m_clock->GetDeltaSeconds();
		m_cameraOrientation.m_yawDegrees -= rightStickPosX * rotateSpeed * g_theGame->m_clock->GetDeltaSeconds();
	}
	if (g_theInput->GetController(0).IsButtonDown(XBOX_BUTTON_START))
	{
		m_cameraPosition = Vec3::ZERO;
		m_cameraOrientation = EulerAngles();
	}
	if (g_theInput->GetController(0).IsButtonDown(XBOX_BUTTON_SHOULDER_LEFT))
	{
		velocity += Vec3(0, 0, walk);
	}
	if (g_theInput->GetController(0).IsButtonDown(XBOX_BUTTON_SHOULDER_RIGHT))
	{
		velocity -= Vec3(0, 0, walk);
	}
	if (g_theInput->GetController(0).IsButtonDown(XBOX_BUTTON_A))
	{
		velocity *= run;
	}

	// MKB
	m_cameraOrientation.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * 0.075f;
	m_cameraOrientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.075f;

	if (g_theInput->WasKeyJustPressed('N'))
	{
		g_currentMap->DebugPossessNext();
		if (GetActor())
		{
			m_isFreefly = !m_isFreefly;
			SetCameraWhenSwitch();
		}
	}

	if (g_theInput->IsKeyDown('W'))
	{
		velocity += forwardDir * walk;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		velocity -= forwardDir * walk;
	}
	if (g_theInput->IsKeyDown('A'))
	{
		velocity += rightDir * walk;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		velocity -= rightDir * walk;
	}
	if (g_theInput->IsKeyDown('Z'))
	{
		velocity += Vec3(0, 0, walk);
	}
	if (g_theInput->IsKeyDown('C'))
	{
		velocity -= Vec3(0, 0, walk);
	}
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		velocity *= run;
	}

	m_cameraOrientation.m_pitchDegrees = Clamp(m_cameraOrientation.m_pitchDegrees, -85.f, 85.f);
	m_cameraPosition += velocity * g_theGame->m_clock->GetDeltaSeconds();
}