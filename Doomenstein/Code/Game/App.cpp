#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"


App* g_theApp = nullptr;
RandomNumberGenerator* g_theRNG = nullptr;
Renderer* g_theRenderer = nullptr;
AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_theFont = nullptr;
Game* g_theGame = nullptr;

App::App()
{

}

App::~App()
{

}

void App::Startup()
{
	InitializeGameConfig("Data/GameConfig.xml");

	Clock::s_theSystemClock = new Clock();


	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = "Doomenstein";
	windowConfig.m_clientAspect = g_gameConfigBlackboard.GetValue("windowAspect", 2.f);
	g_theWindow = new Window(windowConfig);

	RendererConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(renderConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	DevConsoleConfig consoleConfig;
	consoleConfig.m_renderer = g_theRenderer;
	consoleConfig.m_fontFilePath = "Data/Fonts/SquirrelFixedFont";
	consoleConfig.m_camera = new Camera();
	g_theDevConsole = new DevConsole(consoleConfig);

	g_theGame = new Game();

	DebugRenderConfig debugrenderConfig;
	debugrenderConfig.m_renderer = g_theRenderer;

	Clock::s_theSystemClock->TickSystemClock();
	g_theEventSystem->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	DebugRenderSystemStartUp(debugrenderConfig);
	g_theAudio->Startup();
	g_theDevConsole->Startup();

	g_theGame->Startup();

	g_theRNG = new RandomNumberGenerator();
	g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	SubscribeEventCallbackFunction("quit", App::Event_Quit);

	ConsoleTutorial();
}

bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return false;
}

void App::Shutdown()
{
	g_theGame->Shutdown();
	g_theDevConsole->Shutdown();
	g_theAudio->Shutdown();
	DebugRenderSystemShutdown();
	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theEventSystem->Shutdown();

	delete g_theRNG;
	delete g_theGame;
	g_theGame = nullptr;
	delete g_theDevConsole;
	g_theDevConsole = nullptr;
	delete g_theAudio;
	g_theAudio = nullptr;
	delete g_theRenderer;
	g_theRenderer = nullptr;
	delete g_theWindow;
	g_theWindow = nullptr;
	delete g_theInput;
	g_theInput = nullptr;
	delete g_theEventSystem;
	g_theEventSystem = nullptr;

}

void App::Run()
{
	while (!m_isQuitting)
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	BeginFrame();
	Update();
	Render();
	EndFrame();
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return 0;
}

void App::BeginFrame()
{
	Clock::s_theSystemClock->TickSystemClock();
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	DebugRenderBeginFrame();
	g_theAudio->BeginFrame();
	g_theDevConsole->BeginFrame();
}

void App::Update()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_TIDLE))
	{
		g_theDevConsole->ToggleOpen();
	}
	if (g_theInput->IsKeyDown('T'))
	{
		g_theGame->m_clock->SetTimeScale(0.1f);
	}
	if (g_theInput->WasKeyJustReleased('T'))
	{
		g_theGame->m_clock->SetTimeScale(1.f);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		g_theGame->m_clock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		g_theGame->m_clock->StepSingleFrame();
	}

	if (g_theDevConsole->IsOpen() || g_theGame->GetCurrentState() == GameState::ATTRACT_MODE || GetForegroundWindow() != Window::GetMainWindowInstance()->GetHwnd())
	{
		g_theInput->SetCursorMode(false, false);
	}
	else 
	{
		g_theInput->SetCursorMode(true, true);
	}
	g_theGame->Update();
}

void App::Render() const
{
	g_theGame->Render();
	AABB2 screenBound(0.f, 0.f, 1600.f, 800.f);
	g_theDevConsole->Render(screenBound, g_theRenderer);
}

void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	DebugRenderEndFrame();
	g_theAudio->EndFrame();
	g_theDevConsole->EndFrame();
}

void App::ConsoleTutorial()
{
	g_theDevConsole->AddLine(Rgba8::COLOR_TRANSPARENT, "\n");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "			HOW TO PLAY			");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "-----------------");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Space - Start to Start Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "ESC - Back to Exit Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "~ to toggle Dev Console");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "WASD - Left Joystick to move");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Left Mouse to shoot/ fishing - Right shoulder to fish - Right trigger to shoot");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Right Mouse - Left shoulder to scope sniper");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Q/E - D-Pad Up/Down to change weapon ");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "1 - X to equip fishing rod");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "2 - Y to equip newest weapon just bought");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "F - A to buy");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "R - B to reload");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Shift - Left Trigger to sprint");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "-----------------");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "						DEBUG			");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "-----------------");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "P to pause");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "X to toggle free fly mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "O to run 1 frame");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Hold T to enter slow-mo mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "-----------------");
	g_theDevConsole->AddLine(Rgba8::COLOR_TRANSPARENT, "\n");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Type help to see the list of events");
}

void App::InitializeGameConfig(const char* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* element = file.FirstChildElement();

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*element);
}