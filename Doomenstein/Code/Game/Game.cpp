#include "Game/Game.hpp"
#include "Game/Tile.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/PlayerController.hpp"

Map* g_currentMap = nullptr;

Game::Game()
{

}

Game::~Game()
{

}

void Game::Startup()
{
	ActorDefinition::InitializeProjectileActorDefinitions("Data/Definitions/ProjectileActorDefinitions.xml");
	WeaponDefinition::InitializeWeaponDefs("Data/Definitions/WeaponDefinitions.xml");
	ActorDefinition::InitializeActorDefs("Data/Definitions/ActorDefinitions.xml");
	TileDefinition::InitializeTileDefs("Data/Definitions/TileDefinitions.xml");
	MapDefinition::InitializeMapDefs("Data/Definitions/MapDefinitions.xml");

	m_testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");

	m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	m_terrainSprites = new SpriteSheet(*m_spriteSheetTexture, IntVec2(8, 8));
	m_titleTexture = g_theRenderer->CreateOrGetTextureFromFile(g_gameConfigBlackboard.GetValue("title", "Data/Images/title.png").c_str());
	m_fishingRodTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FishingRodCyl.png");

	m_menuMusic = g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("mainMenuMusic", "Data/Audio/Music/MainMenu_InTheDark.mp2"));
	m_gameMusic = g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("gameMusic", "Data/Audio/Music/E1M1_AtDoomsGate.mp2"));
	m_buttonClickSound = g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("buttonClickSound", "Data/Audio/Click.mp3"));
	m_musicVolume = g_gameConfigBlackboard.GetValue("musicVolume", 0.1f);

	m_screenCamera.SetOrthographicView(Vec2(0, 0), Vec2(g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f), g_gameConfigBlackboard.GetValue("screenSizeY", 800.f)));
	m_clock = new Clock(*Clock::s_theSystemClock);

	CreateNewMaps();
	SwitchState(GameState::ATTRACT_MODE);
}

void Game::Shutdown()
{
	TileDefinition::ClearDefinition();
	MapDefinition::ClearDefinition();
	ActorDefinition::ClearDefinition();
	WeaponDefinition::ClearDefinition();

	for (size_t i = 0; i < 2; i++)
	{
		if (m_player[i])
		{
			delete m_player[i];
			m_player[i] = nullptr;
		}
	}
	g_currentMap = nullptr;
}

void Game::Update()
{
	m_secondIntoMode += m_clock->GetDeltaSeconds();

	if (m_currentState == GameState::PLAY_MODE)
	{
		UpdatePlayMode();
	}
	if (m_currentState == GameState::ATTRACT_MODE)
	{
		UpdateAttractMode();
	}
}

void Game::CreateNewMaps()
{
	for (int i = 0; i < (int)m_maps.size(); i++)
	{
		delete m_maps[i];
		m_maps[i] = nullptr;
	}
	m_maps.clear();

	for (size_t i = 0; i < MapDefinition::s_mapDefs.size(); i++)
	{
		m_maps.push_back(new Map(MapDefinition::s_mapDefs[i]));
	}

// 	m_defaultMapName = MapDefinition::GetByName(g_gameConfigBlackboard.GetValue("defaultMap","TestMap"));
// 	Map* map = new Map(m_defaultMapName);
// 	m_maps.push_back(map);
}

void Game::UpdatePlayMode()
{
	DebugAddScreenText(Stringf("[Game Clock] Time: %.1f, FPS: %.1f, Scale: %.1f", m_clock->GetTotalSeconds(), 1.f / m_clock->GetDeltaSeconds(), m_clock->GetTimeScale()),
		Vec2(1010.f, 780.f), 12.5f);

	HandleInput();
	for (int i = 0; i < m_numPlayer; i++)
	{
		if (m_player[i]) 
		{
			m_player[i]->Update();
		}
	}
	
	g_currentMap->Update();

	for (int i = 0; i < m_numPlayer; i++)
	{
		if (m_player[i])
		{
			m_player[i]->UpdateCamera();
		}
	}

	for (int i = 0; i < m_numPlayer; i++)
	{
		if (m_player[i])
		{
			Mat44 playerTransform = m_player[i]->GetModelMatrix();
			g_theAudio->UpdateListener(i, playerTransform.GetTranslation3D(), playerTransform.GetIBasis3D(), playerTransform.GetKBasis3D());
		}
	}
}

void Game::UpdateAttractMode()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		if (m_currentState == GameState::ATTRACT_MODE)
		{
			if (m_secondIntoMode > 0.2f)
			{
				if (m_numPlayer == 0)
				{
					g_theApp->m_isQuitting = true;
				}
			}
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		PlaySound(m_buttonClickSound);
		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i] && m_player[i]->m_controllerIndex == -1)
			{
				SwitchState(GameState::PLAY_MODE);
				return;
			}
		}
		if (m_numPlayer == 0)
		{
			if (!m_player[0])
			{
				m_player[0] = new PlayerController();
				m_player[0]->m_controllerIndex = -1;
				m_numPlayer++;
			}
		}
		else
		{
			if (m_player[0]->m_controllerIndex == -1)
			{
				return;
			}
			if (!m_player[1])
			{
				m_player[1] = new PlayerController();
				m_player[1]->m_controllerIndex = -1;
				m_numPlayer++;
			}
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		if (m_numPlayer == 1)
		{
			if (m_player[0]->m_controllerIndex != -1)
			{
				return;
			}
		}
		PlaySound(m_buttonClickSound);
		if (m_numPlayer == 0)
		{
			return;
		}
		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i] && m_player[i]->m_controllerIndex == -1)
			{
				delete m_player[i];
				m_player[i] = nullptr;
				if (i == 0 && m_numPlayer > 1)
				{
					m_player[0] = m_player[1];
					m_player[1] = nullptr;
				}
				break;
			}
		}
		if (m_numPlayer > 0)
		{
			m_numPlayer--;
		}
	}


	if (g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
	{
		PlaySound(m_buttonClickSound);
		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i] && m_player[i]->m_controllerIndex == 0)
			{
				SwitchState(GameState::PLAY_MODE);
				return;
			}
		}
		if (m_numPlayer == 0)
		{
			if (!m_player[0])
			{
				m_player[0] = new PlayerController();
				m_player[0]->m_controllerIndex = 0;
				m_numPlayer++;
			}
		}
		else
		{
			if (m_player[0]->m_controllerIndex == 0)
			{
				return;
			}
			if (!m_player[1])
			{
				m_player[1] = new PlayerController();
				m_player[1]->m_controllerIndex = 0;
				m_numPlayer++;
			}
		}
	}
	if (g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		if (m_numPlayer == 1)
		{
			if (m_player[0]->m_controllerIndex != 0)
			{
				return;
			}
		}
		PlaySound(m_buttonClickSound);
		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i] && m_player[i]->m_controllerIndex == 0)
			{
				delete m_player[i];
				m_player[i] = nullptr;
				if (i == 0 && m_numPlayer > 1)
				{
					m_player[0] = m_player[1];
					m_player[1] = nullptr;
				}
				break;
			}
		}
		if (m_numPlayer > 0)
		{
			m_numPlayer--;
		}
	}
}

void Game::Render() const
{
	if (m_currentState == GameState::PLAY_MODE)
	{
		RenderPlayMode();

		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i])
			{
				m_player[i]->RenderHUD();
				DebugRenderWorld(*m_player[i]->GetCamera());
			}
		}

		DebugRenderScreen(m_screenCamera);
	}
	if (m_currentState == GameState::ATTRACT_MODE)
	{
		RenderAttractMode();
		DebugRenderScreen(m_screenCamera);
	}
	RenderScreenWorld();
}

void Game::RenderPlayMode() const
{
	g_theRenderer->ClearScreen(Rgba8(20, 20, 20, 255));
	for (int i = 0; i < m_numPlayer; i++)
	{
		g_currentMap->Render(i);
	}

}

void Game::RenderAttractMode() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 255));
	g_theRenderer->BeginCamera(m_screenCamera);
	g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindShader(nullptr);

	std::vector<Vertex_PCU> verts;
	verts.reserve(100);
	AABB2 bottomBox = AABB2(360.f, 0.f, 1200.f, 200.f);

	AABB2 singlePlayerBoxTitle = AABB2(360.f, 400.f, 1200.f, 500.f);
	AABB2 singlePlayerBoxInput = AABB2(360.f, 340.f, 1200.f, 400.f);
	AABB2 singlePlayerBoxDetail = AABB2(360.f, 230.f, 1200.f, 330.f);

	AABB2 p1MultiplayerBoxTitle = AABB2(360.f, 650.f, 1200.f, 750.f);
	AABB2 p1MultiplayerBoxInput = AABB2(360.f, 610.f, 1200.f, 650.f);
	AABB2 p1MultiplayerBoxDetail = AABB2(360.f, 500.f, 1200.f, 600.f);

	AABB2 p2MultiplayerBoxTitle = AABB2(360.f, 170.f, 1200.f, 270.f);
	AABB2 p2MultiplayerBoxInput = AABB2(360.f, 130.f, 1200.f, 170.f);
	AABB2 p2MultiplayerBoxDetail = AABB2(360.f, 20.f, 1200.f, 120.f);

	if (m_numPlayer == 0)
	{	
		std::vector<Vertex_PCU> titles;
		AddVertsForAABB2D(titles, AABB2(300.f, 170.f, 1300.f, 720.f), Rgba8::COLOR_WHITE);
		g_theRenderer->BindTexture(m_titleTexture);
		g_theRenderer->DrawVertexArray((int)titles.size(), titles.data());

		g_theFont->AddVertsForTextInBox2D(verts, bottomBox, 20.f, "Press Space To Join with MKB\n Press Start To Join with Controller\n Press ESC To Quit");
	}
	else if (m_numPlayer == 1)
	{
		if (m_player[0]->m_controllerIndex == -1)
		{
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxTitle  , 60.f, "Player 1", Rgba8::COLOR_RED);
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxInput  , 30.f, "Mouse And Keyboard", Rgba8::COLOR_GREEN);
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxDetail , 20.f, "Space To Start Game\n ESC to Leave  Game\n Start To Join with Controller");

		}
		else
		{
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxTitle  , 60.f, "Player 1", Rgba8::COLOR_RED);
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxInput  , 30.f, "Controller", Rgba8::COLOR_CYAN);
			g_theFont->AddVertsForTextInBox2D(verts, singlePlayerBoxDetail , 20.f, "Start To Start Game\n Back to Leave  Game\n Space To Join with MKB");
		}
	}
	else
	{
		if (m_player[0]->m_controllerIndex == -1)
		{
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxTitle , 60.f, "Player 1", Rgba8::COLOR_RED);
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxInput , 30.f, "Mouse And Keyboard", Rgba8::COLOR_GREEN);
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxDetail, 20.f, "Space To Start Game\n ESC to Leave  Game");

			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxTitle, 60.f, "Player 2", Rgba8::COLOR_YELLOW);
			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxInput, 30.f, "Controller", Rgba8::COLOR_CYAN);
			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxDetail, 20.f, "Start To Start Game\n Back to Leave  Game");

		}
		else
		{
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxTitle, 60.f, "Player 1", Rgba8::COLOR_RED);
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxInput, 30.f, "Controller", Rgba8::COLOR_CYAN);
			g_theFont->AddVertsForTextInBox2D(verts, p1MultiplayerBoxDetail, 20.f, "Start To Start Game\n Back to Leave  Game");

			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxTitle, 60.f, "Player 2", Rgba8::COLOR_YELLOW);
			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxInput, 30.f, "Mouse And Keyboard", Rgba8::COLOR_GREEN);
			g_theFont->AddVertsForTextInBox2D(verts, p2MultiplayerBoxDetail, 20.f, "Space To Start Game\n ESC to Leave  Game");
		}
	}

	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderScreenWorld() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	g_theRenderer->SetDepthStencilMode(DepthMode::DISABLED);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetSamplerMode(SampleMode::POINT_CLAMP);

	AABB2 screenBound = AABB2(m_screenCamera.GetOrthographicBottomLeft(), m_screenCamera.GetOrthographicTopRight());

	if (m_clock->IsPaused())
	{
		std::vector<Vertex_PCU> UIVerts;
		UIVerts.reserve(6);
		AddVertsForAABB2D(UIVerts, screenBound, Rgba8::DARK);
		g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)UIVerts.size(), UIVerts.data());

		std::vector<Vertex_PCU> UITextVerts;
		g_theFont->AddVertsForText2D(UITextVerts, Vec2(720.f, 600.f), 20.f, "PAUSED");
		g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)UITextVerts.size(), UITextVerts.data());
	}
	g_theDevConsole->Render(screenBound, g_theRenderer);
	g_theRenderer->EndCamera(m_screenCamera);
}


void Game::PlaySound(SoundPlaybackID sound, bool loop)
{
	if (sound == m_buttonClickSound)
	{
		g_theAudio->StartSound(sound, false, m_musicVolume);
		return;
	}
	if (sound == m_currentMusicPlayBackID && loop)
	{
		return;
	}
	if (m_currentMusicPlayBackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_currentMusicPlayBackID);
	}
	m_currentMusicPlayBackID = g_theAudio->StartSound(sound, loop, m_musicVolume);
}

void Game::GameRestart()
{
	Shutdown();
	Startup();
	SwitchState(GameState::PLAY_MODE);
}


void Game::HandleInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		g_theGame->SwitchState(GameState::ATTRACT_MODE);
	}
}

void Game::EnterCurrentState()
{
	switch (m_currentState)
	{
	case GameState::NONE:
		break;
	case GameState::ATTRACT_MODE:
		PlaySound(m_menuMusic, true);
		m_numPlayer = 0;
		break;
	case GameState::PLAY_MODE:
		g_currentMap = m_maps[2];
		g_currentMap->StartUp();
		if (m_numPlayer == 1)
		{
			AABB2 cameraBox = AABB2(0.f, 0.f, 1.f, 1.f);
			m_player[0]->Initialize(cameraBox);
			g_currentMap->SpawnPlayer(0);
		}
		else
		{
			float minY = 1.f / m_numPlayer;
			for (int i = 0; i < m_numPlayer; i++)
			{
				AABB2 cameraBox = AABB2(0.f, minY - minY * i, 1.f, 1.f - minY * i);
				m_player[i]->Initialize(cameraBox);
				g_currentMap->SpawnPlayer(i);
			}
		}

		PlaySound(m_gameMusic, true);

		// DEBUG
		g_currentMap->AddAllActorsCanBePossess();
		break;
	}
}

void Game::ExitCurrentState()
{
	switch (m_currentState)
	{
	case GameState::NONE:
		break;
	case GameState::ATTRACT_MODE:

		break;
	case GameState::PLAY_MODE:
		g_currentMap->Shutdown();
		for (int i = 0; i < m_numPlayer; i++)
		{
			if (m_player[i]) 
			{
				delete m_player[i];
				m_player[i] = nullptr;
			}
		}
		//g_currentMap = nullptr;
		break;
	}
	if (m_currentMusicPlayBackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_currentMusicPlayBackID);
	}
}

void Game::SwitchState(GameState state)
{
	if (state == m_currentState)
	{
		return;
	}
	m_secondIntoMode = 0.f;
	ExitCurrentState();
	m_prevState = m_currentState;
	m_currentState = state;
	EnterCurrentState();
}

GameState const Game::GetCurrentState()
{
	return m_currentState;
}