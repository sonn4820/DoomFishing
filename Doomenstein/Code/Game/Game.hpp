#pragma once
#include "Game/GameCommon.hpp"

class Entity;
class PlayerController;
class Prop;
class Map;
struct MapDefinition;

extern Map* g_currentMap;

enum class GameState 
{
	NONE,
	ATTRACT_MODE,
	PLAY_MODE,
	COUNT
};
class Game
{
public:
	Game();
	~Game();

	void Startup();
	void Update();
	void Render() const;
	void Shutdown();

	Camera m_screenCamera;
	GameState m_currentState = GameState::NONE;
	GameState m_prevState = GameState::NONE;

	// STATE
	void EnterCurrentState();
	void ExitCurrentState();
	void SwitchState(GameState state);
	GameState const GetCurrentState();
	// GAME RESTART
	void GameRestart();

public:
	Clock* m_clock = nullptr;
	MapDefinition* m_defaultMapName = nullptr;
	std::vector<Map*> m_maps;
	int m_currentLevelNum = 0;

	Texture* m_testTexture = nullptr;
	Texture* m_spriteSheetTexture = nullptr;
	Texture* m_titleTexture = nullptr;
	Texture* m_fishingRodTexture = nullptr;
	SpriteSheet* m_terrainSprites = nullptr;

	float m_musicVolume = 0.1f;
	SoundPlaybackID m_menuMusic;
	SoundPlaybackID m_gameMusic;
	SoundPlaybackID m_buttonClickSound;

	PlayerController* m_player[2];
	int m_numPlayer = 0;
private:
	float m_screenShakeAmount = 0.0f;
	float m_secondIntoMode = 0.f;
	SoundPlaybackID m_currentMusicPlayBackID = MISSING_SOUND_ID;
	// START
	void CreateNewMaps();

	// UPDATE
	void UpdatePlayMode();
	void UpdateAttractMode();
	void HandleInput();

	// RENDER
	void RenderPlayMode() const;
	void RenderAttractMode() const;
	void RenderScreenWorld() const;

	// AUDIO
	void PlaySound(SoundPlaybackID sound, bool loop = false);
};