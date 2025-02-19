#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Actor.hpp"

struct WeaponAnimation
{
	std::string m_name = "";
	Shader* m_animShader = nullptr;
	Texture* m_spriteSheet = nullptr;
	IntVec2 m_cellCount = IntVec2();
	std::string m_playbackMode = "Once";
	float m_duration = 0.f;
	int m_startFrame = 0;
	int m_endFrame = 0;
};
struct WeaponDefinition
{
	std::string m_name = "";
	float m_refireTime = 0.f;
	int m_price = 0;
	int m_magPrice = 0;
	int m_magazine = 0;
	float m_moveSpeed = 1.f;
	bool m_canScope = false;
	bool m_hasGunSmoke = false;
	bool m_isFlash = true;
	float m_smokeRadius = 0.025f;
	float m_smokeSpeed = 1.f;
	Vec3 m_smokeOffset;
	Texture* m_scopeTexture = nullptr;
	Texture* m_smokeTexture = nullptr;

	// RAY
	float m_rayCount = 0.f;
	float m_rayCone = 0.f;
	float m_rayRange = 0.f;
	FloatRange m_rayDamage = FloatRange();
	float m_rayImpulse = 0.f;

	// PROJECTILE
	int m_projectileCount = 0;
	float m_projectileCone = 0.f;
	float m_projectileSpeed = 0.f;
	ActorDefinition* m_projectileActor;

	// MELEE
	int m_meleeCount = 0;
	float m_meleeArc = 0.f;
	float m_meleeRange = 0.f;
	FloatRange m_meleeDamage = FloatRange();
	float m_meleeImpulse = 0.f;

	// HUD
	Shader* m_shader = nullptr;
	Texture* m_baseTexture = nullptr;
	Texture* m_reticleTexture = nullptr;
	IntVec2 m_reticleSize = IntVec2();
	IntVec2 m_spriteSize = IntVec2();
	Vec2 m_spritePivot = Vec2();

	// ANIMATION 
	SpriteSheet* m_weaponSpriteSheet = nullptr;
	std::vector<WeaponAnimation> m_animation;

	// SOUND
	std::vector<std::string> m_sound;
	std::vector<std::string> m_soundName;
	bool m_overrideCurrentSound = true;
public:
	WeaponDefinition(XmlElement& element);
	void SetHUD(XmlElement& element);
	void SetAnimation(XmlElement& element);
	void SetSound(XmlElement& element);
	static void InitializeWeaponDefs(char const* filePath);
	static WeaponDefinition* GetByName(std::string const& name);
	static void ClearDefinition();
	static std::vector<WeaponDefinition*> s_weaponDefs;
};
class Weapon
{
public:
	Weapon(WeaponDefinition* weaponDef);
	virtual ~Weapon();
	void Update();
	void Render(Camera& camera) const;
	void Fire();
	Vec3 GetRandomDirectionInCone(float coneRadius);

	void Reload();
	void PullBackFishingRod();
	void PlayAnimationByName(std::string name);
	void PlaySoundByName(std::string name, bool overrideTheCurrentSound = true);
	bool HasAmmo() const;
	void StopAllFishingTimer();

public:
	WeaponDefinition* m_weaponDef = nullptr;
	ActorUID m_holder;
	float m_refireTimer = 0.f;
	// Sound
	std::vector<Sound> m_soundsList;
	Sound m_currentSound;
	bool m_isScoping = false;
	int m_currentMagCount = 0;
	int m_currentAmmoCount = 0;
	bool m_isReloading = false;
	bool m_soundOverrided = true;
	WeaponAnimation m_currentAnimation;
	Clock* m_animationClock = nullptr;

	Vec3 m_lastHolderFishingPos;

	bool m_isFishingRod = false;
	bool m_isCurrentlyFishing = false;
	ActorUID m_bait = ActorUID::INVALID;

private:
	void AddBulletFromTotalToMag();
private:
	Vec3 m_lastHitPos;
	Vec3 m_lastSmokeStartPos;
	Timer* m_smokeTimer = nullptr;
	Timer* m_reloadingTimer = nullptr;
	Timer* m_fishingThrowTimer = nullptr;
	Timer* m_fishingPullTimer = nullptr;
};