#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Controller.hpp"
#include "Game/Weapon.hpp"
#include "Game/Map.hpp"

class PlayerController : public Controller
{
public:
	virtual ~PlayerController();
	virtual bool IsPlayer() const;
	virtual void Possess(ActorUID actorUID) override;
	virtual void Update();
	virtual void DamagedBy(ActorUID dealer) override;

	void Initialize(AABB2 cameraBox);

	void UpdateInput();
	void UpdateCamera();
	void UpdateFreeFlyMode();
	void UpdateProcessedMode();
	void UpdateControllerInput(int controllerID);
	void UpdateMKBInput();

	void RenderHUD() const;
	void InteractionWithWorld();
	void Interact();
	void AddMoney(int amount);
	void Reset();

	Camera* GetCamera() const;
	Camera* GetScreenCamera() const;
	Mat44 GetModelMatrix() const;
public:
	Camera* m_playerCamera = nullptr;
	Camera* m_playerScreenCamera = nullptr;
	bool m_isFreefly = false;
	Vec3 m_cameraPosition;
	EulerAngles m_cameraOrientation;
	float m_recordedActorPitch = 0.f;
	float m_recordedActorYaw = 0.f;
	int m_controllerIndex = -1;
	int m_numPlayerKilled = 0;
	int m_numPlayerDeath = 0;
	int m_killStreak = 0;
	int m_money = 0;
private:
	float m_eyeHeight = 0.f;
	RaycastResult m_currentInteraction;
private:
	void SetCameraWhenSwitch();
	SpriteAnimDefinition GetCurrentWeaponAnim() const;
	AABB2 GetTextPositionOnHUD(float valueZeroToOneOnX, float valueZeroToOneOnY, float cellHeight, float cellAspect = 1.f) const;
	Vec2 GetTextMinPositionOnScreen(float valueZeroToOneOnX, float valueZeroToOneOnY) const;
private:
	Timer* m_flashRedTimer = nullptr;
};