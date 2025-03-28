#pragma once
#include "Engine/Window/Window.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/FileUtils.hpp"
#include <string>
#include <vector>
#include <mutex>

class Renderer;
class BitmapFont;
class Camera;
class Timer;
struct AABB2;

class DevConsole;
extern DevConsole* g_theDevConsole; 

#ifdef ERROR
#undef ERROR
#endif // ERROR

struct DevConsoleConfig
{
	Renderer* m_renderer = nullptr;
	Camera* m_camera = nullptr;
	std::string m_fontFilePath = "Data/Fonts/SquirrelFixedFont";
	float m_numLinesVisible = 40.f;
	float m_fontAspect = 0.7f;
	int m_maxCommandHistory = 128;
	bool m_startOpen = false;

};
struct DevConsoleLine
{
	Rgba8 m_color = Rgba8::COLOR_WHITE;
	std::string m_text;
	int m_frameNumber = 0;
	double m_timestamp = 0.0f;
};

struct Foo
{
	virtual bool Laugh(EventArgs& args) = 0;
};

struct Faa : public Foo
{
	bool Laugh(EventArgs& args) override;
	int m_ID = 0;
};

struct Fee : public Foo
{
	bool Laugh(EventArgs& args) override;
};

class DevConsole 
{
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();
	
	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void Execute(std::string const& consoleCommandText, bool echoCommand = true);
	void ExecuteXmlCommandScriptNode(XmlElement const& commandScriptXmlElement);
	void ExecuteXmlCommandScriptFile(std::string& commandScriptXmlFilePathName);
	void AddLine(Rgba8 const& color, std::string const& text);
	void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;

	int GetCurrentFrame();
	void ToggleOpen();
	bool IsOpen();

	static const Rgba8 ERROR;
	static const Rgba8 WARNING;
	static const Rgba8 SUCCESS;
	static const Rgba8 INFO_MAJOR;
	static const Rgba8 INFO_MINOR;
	static const Rgba8 COMMAND_ECHO;
	static const Rgba8 COMMAND_REMOTE_ECHO;
	static const Rgba8 INPUT_TEXT;
	static const Rgba8 INPUT_INSERTION_POINT;

	static bool Event_KeyPressed(EventArgs& args);
	static bool Event_CharInput(EventArgs& args);
	static bool Command_Clear(EventArgs& args);
	static bool Command_Echo(EventArgs& args);
	static bool Command_Help(EventArgs& args);
	static bool Command_SetTimeScale(EventArgs& args);
	static bool Command_Print(EventArgs& args);

	// SD 4 A2 functions
	static bool Command_TestBinaryFileLoad(EventArgs& args);
	static bool Command_TestBinaryFileSave(EventArgs& args);

	// SD 4 A3 Testing function
	static bool Command_ExecuteXML(EventArgs& args);
	static bool Command_Test_ExecuteXML(EventArgs& args);
	static bool Command_TestFunction(EventArgs& args);
	static bool Command_TestSubFunction(EventArgs& args);
	static bool Command_TestUnsubFunction(EventArgs& args);
	static bool Command_TestUnsubMethod(EventArgs& args);
	bool Test_UnsubMemberMethod(int index);

	std::vector<Foo*> m_test_Foo_List;
	Faa* m_faa1 = nullptr;
	Faa* m_faa2 = nullptr;
	Fee* m_fee = nullptr;

	BitmapFont* m_font = nullptr;

protected:
	void Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect = 1.f) const;

protected:
	DevConsoleConfig			m_config;
	Camera*						m_camera = nullptr;
	Timer*						m_insertionPointBlinkTimer = nullptr;
	std::vector<DevConsoleLine> m_lines;
	std::vector<std::string>	m_commandHistory;
	std::string					m_inputText;
	bool						m_showFrameAndTime = true;
	bool						m_isOpen = false;
	bool						m_insertionPointVisible = true;
	int							m_insertionPointPosition = 0;
	int							m_historyIndex = 1;
	int							m_frameNumber = 0;
	int							m_scrollOffset = 0;
};

void AppendTestFileBufferData(BufferWriter& bufferWriter, EndianMode endianMode);
void ParseTestFileBufferData(BufferParser& bufferParser, EndianMode endianMode);