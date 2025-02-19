#pragma once
#include "Game/GameCommon.hpp"

class Map;
class Actor;
class Controller;
class PlayerController;
class AIController;
class Weapon;
struct WeaponDefinition;

struct ActorAnimationGroup
{
	// ANIMATION GROUP
	std::string m_animName = "";
	bool m_scaleBySpeed = false;
	float m_secondsPerFrame = 1.f;
	std::string m_playbackMode = "Once";
	// DIRECTION (sub from ANIMATION GROUP)
	std::vector<Vec3> m_directions;

	// ANIMATION (sub from DIRECTION)
	std::vector<int> m_startFrame;
	std::vector<int> m_endFrame;
	float m_duration = 0;
};
struct Sound
{
	std::string m_name = "";
	SoundID m_soundID;
	SoundPlaybackID m_soundPlaybackID;
};
struct ActorUID
{
	ActorUID() = default;
	ActorUID(unsigned int salt, unsigned int index);
	unsigned int m_data;
	int GetIndex() const;
	bool IsValid() const;
	Actor* GetActor() const;
	Actor* operator->() const;
	bool operator==(ActorUID otherUID) const;
	bool operator!=(ActorUID otherUID) const;
	static ActorUID INVALID;
};
struct ActorDefinition
{
	// BASE
	std::string m_name = "";
	std::string m_faction = "";
	bool m_visible = false;
	bool m_dieOnSpawn = false;
	bool m_canKill = true;
	int m_health = 1;
	int m_killMoney = 0;
	float m_corpseLifetime = 0.f;
	bool m_canBePossesed = false;
	bool m_isRenderedOffset = false;
	Rgba8 m_solidColor = Rgba8::COLOR_WHITE;
	Rgba8 m_wireframeColor = Rgba8::COLOR_WHITE;

	// COLLISION
	float m_physiscHeight = 0.f;
	float m_physiscRadius = 0.f;
	bool m_collideWithWorld = false;
	bool m_collideWithActors = false;
	bool m_dieOnCollide = false;
	bool m_stopPhysicsOnCollide = false;
	FloatRange m_damageOnCollide = FloatRange(0, 0);
	float m_impulseOnCollide = 0.f;

	// PHYSICS
	bool m_simulated = false;
	bool m_flying = false;
	bool m_hasGravity = false;
	float m_walkSpeed = 0.f;
	float m_runSpeed = 0.f;
	float m_drag = 0.f;
	float m_turnSpeed = 0.f;

	// CAMERA
	float m_eyeHeight = 0.f;
	float m_cameraFOV = 60.f;

	// AI
	bool m_AIEnabled = false;
	float m_AISightRadius = 0.f;
	float m_AIAngle = 0.f;
	float m_detectionRate = 1.f;
	bool m_useHeatmap = true;

	// VISUAL
	Vec2 m_size = Vec2(1.f, 1.f);
	Vec2 m_pivot = Vec2(0.5f, 0.5f);
	std::string m_billboardType = "None";
	bool m_renderLit = false;
	bool m_renderRounded = false;
	Shader* m_shader = nullptr;
	Texture* m_spriteSheet = nullptr;
	IntVec2 m_cellCount = IntVec2(1, 1);

	// ANIMATION
	std::vector<ActorAnimationGroup> m_animationGroups;

	// SOUND
	std::vector<Sound> m_soundList;

	// WEAPON
	std::vector<WeaponDefinition*> m_inventory;

public:
	ActorDefinition(XmlElement& element);
	void SetCollision(XmlElement& element);
	void SetPhysics(XmlElement& element);
	void SetCamera(XmlElement& element);
	void SetAI(XmlElement& element);
	void SetVisual(XmlElement& element);
	void SetAnimationGroup(XmlElement& element);
	void SetDirection(XmlElement& element, ActorAnimationGroup& group);
	void SetAnimation(XmlElement& element, ActorAnimationGroup& group);
	void SetSound(XmlElement& element);
	void SetWeapons(XmlElement& element);
	static void InitializeActorDefs(char const* filePath);
	static void InitializeProjectileActorDefinitions(char const* filePath);
	static void ClearDefinition();
	static ActorDefinition* GetByName(std::string const& name);
	static std::vector<ActorDefinition*> s_actorDefs;
};

class Actor
{
public:
	Actor(ActorDefinition* actorDef, Map* map, unsigned int currentSalt, unsigned int index);
	virtual ~Actor();
	virtual void Update();
	virtual void Render(Camera* camera = nullptr) const;
	virtual void Die();

	Mat44 GetModelMatrix() const;
	IntVec2 GetTileLocation() const;
	Vec2 GetColliderCenterXY();
	FloatRange GetColliderMinMaxZ();
	Vec3 GetControllerForward() const;
	Vec3 GetActorForward() const;

	void SetActorPosition(Vec2 centerXY, FloatRange minMaxZ);
	void UpdatePhysics();
	void UpdateVisuals();
	void TakeDamage(int damage, ActorUID damageDealer);
	void AddForce(Vec3 dir, float strength);
	void AddImpulse(Vec3 dir, float strength);
	void OnCollide(Actor* collidedActor);
	void OnPossessed(Controller* controller);
	void OnUnpossessed();
	void MoveInDirection(Vec3 dir, float speed);
	void TurnInDirection(float goalAngle, float maxDeltaAngle);
	void Attack();
	void EquipWeapon(int index);
	Weapon* HasWeapon(std::string name) const;

	void PlayAnimationByName(std::string name);
	void PlaySoundByName(std::string name, bool overrideTheCurrentSound = false, float volumne = 1.f, float speed = 1.f, float balance = 0.f);
public:
	// ID and Refs
	ActorUID m_UID;
	ActorDefinition* m_actorDef = nullptr;
	Map* m_map = nullptr;

	// Basic Info
	Vec3 m_position;
	EulerAngles m_orientationDegrees;
	Vec3 m_velocity;
	Vec3 m_acceleration;
	EulerAngles m_angularVelocity;
	bool m_isDead = false;
	bool m_isStatic = false;
	bool m_isFlying = false;
	bool m_isGarbage = false;
	bool m_hasGravity = false;
	float m_gravityModifier = 1.f;
	float m_corpseTimer = 0.f;
	int m_currentHealth = 0;

	// Weapon
	std::vector<Weapon*> m_weaponInventory;
	Weapon* m_currentWeapon = nullptr;
	int m_currentWeaponIndex = 0;

	// Controller
	Controller* m_controller = nullptr;
	AIController* m_AIController = nullptr;

	// Projectile
	ActorUID m_owner;

	// Render
	SpriteSheet* m_actorAnimSpriteSheet = nullptr;
	ActorAnimationGroup* m_currentAnimation = nullptr;
	Clock* m_animationClock = nullptr;
	Vec2 m_sizeScale = Vec2(1.f, 1.f);
	Timer* m_shrinkTimer = nullptr;

	// Sound
	Sound m_currentSound;

	//	Bait
	bool m_hasFishAttached = false;

private:
	Vec3 GetDirectionOfCamera(Camera& camera) const;
	SpriteAnimDefinition GetSpriteAnimDefBasedOnDirection(Vec3 direction) const;
private:
};

typedef std::vector<Actor*> ActorList;