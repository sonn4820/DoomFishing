#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Core/FileUtils.hpp"

#ifdef ERROR
#undef ERROR
#endif // ERROR

#include <algorithm>
#include <cctype>
#include <string>


DevConsole* g_theDevConsole = nullptr;

const Rgba8 DevConsole::ERROR = Rgba8(255, 70, 70);
const Rgba8 DevConsole::WARNING = Rgba8(255, 255, 0);
const Rgba8 DevConsole::SUCCESS = Rgba8(0, 255, 0);
const Rgba8 DevConsole::INFO_MAJOR = Rgba8(0, 255, 255);
const Rgba8 DevConsole::INFO_MINOR = Rgba8(255, 255, 255);
const Rgba8 DevConsole::INPUT_TEXT = Rgba8(0, 255, 0);
const Rgba8 DevConsole::INPUT_INSERTION_POINT = Rgba8(200, 255, 180);
const Rgba8 DevConsole::COMMAND_ECHO = Rgba8(255, 0, 255);
const Rgba8 DevConsole::COMMAND_REMOTE_ECHO = Rgba8(0, 0, 0);


bool DevConsole::Event_KeyPressed(EventArgs& args)
{
	if (!g_theDevConsole->IsOpen())
	{
		return false;
	}
	unsigned char keyCode = args.GetValue<unsigned char>("KeyCode", 0);
	if (keyCode == KEYCODE_TIDLE)
	{
		g_theDevConsole->ToggleOpen();
		return false;
	}
	if (keyCode == KEYCODE_ENTER)
	{
		if (g_theDevConsole->m_inputText.size() <= 0)
		{
			g_theDevConsole->ToggleOpen();
			return false;
		}

		g_theDevConsole->Execute(g_theDevConsole->m_inputText, true);
		g_theDevConsole->m_commandHistory.push_back(g_theDevConsole->m_inputText);
		g_theDevConsole->m_insertionPointPosition = 0;
		g_theDevConsole->m_historyIndex = (int)g_theDevConsole->m_commandHistory.size();
		g_theDevConsole->m_inputText.clear();
	}
	if (keyCode == KEYCODE_LEFTARROW)
	{
		if (g_theDevConsole->m_insertionPointPosition <= 0)
		{
			return false;
		}
		g_theDevConsole->m_insertionPointPosition--;
	}
	if (keyCode == KEYCODE_RIGHTARROW)
	{
		if (g_theDevConsole->m_insertionPointPosition >= (int)g_theDevConsole->m_inputText.size())
		{
			return false;
		}
		g_theDevConsole->m_insertionPointPosition++;
	}
	if (keyCode == KEYCODE_UPARROW)
	{
		if ((int)g_theDevConsole->m_commandHistory.size() <= 0)
		{
			return false;
		}
		if (g_theDevConsole->m_historyIndex == 0)
		{
			return false;
		}
		g_theDevConsole->m_historyIndex--;
		g_theDevConsole->m_inputText = g_theDevConsole->m_commandHistory[g_theDevConsole->m_historyIndex];
		g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();
	}
	if (keyCode == KEYCODE_DOWNARROW)
	{
		if ((int)g_theDevConsole->m_commandHistory.size() <= 0)
		{
			return false;
		}
		if (g_theDevConsole->m_historyIndex == (int)g_theDevConsole->m_commandHistory.size() - 1)
		{
			return false;
		}
		g_theDevConsole->m_historyIndex++;
		g_theDevConsole->m_inputText = g_theDevConsole->m_commandHistory[g_theDevConsole->m_historyIndex];
		g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();

	}
	if (keyCode == KEYCODE_HOME)
	{
		g_theDevConsole->m_insertionPointPosition = 0;
	}
	if (keyCode == KEYCODE_END)
	{
		g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();
	}
	if (keyCode == KEYCODE_DELETE)
	{
		if (g_theDevConsole->m_insertionPointPosition == (int)g_theDevConsole->m_inputText.size())
		{
			return false;
		}
		g_theDevConsole->m_inputText.erase(g_theDevConsole->m_inputText.begin() + g_theDevConsole->m_insertionPointPosition);
	}
	if (keyCode == KEYCODE_BACKSPACE)
	{
		if (g_theDevConsole->m_inputText.size() <= 0)
		{
			return false;
		}
		if (g_theDevConsole->m_insertionPointPosition != 0)
		{
			g_theDevConsole->m_inputText.erase(g_theDevConsole->m_inputText.begin() + g_theDevConsole->m_insertionPointPosition - 1);
			g_theDevConsole->m_insertionPointPosition--;
		}
	}
	if (keyCode == KEYCODE_ESCAPE)
	{
		if (g_theDevConsole->m_inputText.size() <= 0)
		{
			FireEvent("clear");
			g_theDevConsole->ToggleOpen();
			return false;
		}
		g_theDevConsole->m_inputText.clear();
		g_theDevConsole->m_insertionPointPosition = 0;
	}
	return true;
}

bool DevConsole::Event_CharInput(EventArgs& args)
{
	if (!g_theInput || !g_theDevConsole->IsOpen())
	{
		return false;
	}
	unsigned char keyCode = args.GetValue<unsigned char>("KeyCode", 0);
	if (keyCode < 32 || keyCode > 126)
	{
		return false;
	}
	g_theDevConsole->m_inputText.insert(g_theDevConsole->m_inputText.begin() + g_theDevConsole->m_insertionPointPosition, keyCode);
	g_theDevConsole->m_insertionPointPosition++;
	return false;
}

bool DevConsole::Command_Clear(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->m_lines.clear();
	g_theDevConsole->m_scrollOffset = 0;
	return false;
}

bool DevConsole::Command_Echo(EventArgs& args)
{
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, args.GetValue<std::string>("message", ""));

	return false;
}

bool DevConsole::Command_Help(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "LIST OF REGISTERED EVENTS");
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "_____________________________");
	std::vector<std::string> eventList = g_theEventSystem->GetAllRegisteredEvent();
	for (size_t i = 0; i < eventList.size(); i++)
	{
		g_theDevConsole->AddLine(DevConsole::INFO_MINOR, std::to_string(i + 1) + ". " + eventList[i]);
	}
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "_____________________________");
	return false;
}

bool DevConsole::Command_SetTimeScale(EventArgs& args)
{
	if (!args.IsKeyNameValid("s"))
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, "ERROR: Arguments are missing or incorrect, please use timescale s=<value>");
	}
	else
	{
		std::string result = args.GetValue<std::string>("s", "1.0");
		float resultInFloat = (float)atof(result.c_str());
		Clock::s_theSystemClock->SetTimeScale(resultInFloat);
		g_theDevConsole->AddLine(DevConsole::SUCCESS, "Time Scale is set to: " + result);
	}

	return false;
}

bool DevConsole::Command_Print(EventArgs& args)
{
	if (!args.IsKeyNameValid("p"))
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, "ERROR: Arguments are missing or incorrect, please use p=<string>");
	}
	else
	{
		std::string result = args.GetValue<std::string>("p", "");
		g_theDevConsole->AddLine(DevConsole::SUCCESS, result);
	}
	return false;
}
bool DevConsole::Command_TestBinaryFileLoad(EventArgs& args)
{
	UNUSED(args);

	const char* testFilePath = "Saves/Test.binary";
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Loading test binary file '%s'...\n", testFilePath));

	// Load from disk
	std::vector<unsigned char> buffer;
	bool success = LoadBinaryFileToExistingBuffer(testFilePath, buffer);
	if (!success)
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("FAILED to load file %s\n", testFilePath));
		return false;
	}

	// Parse and verify
	BufferParser bufParser(buffer);
	//GUARANTEE_RECOVERABLE( bufParser.GetRemainingSize() == TEST_BUF_SIZE, "ERROR"); // should be 208 Bytes total (two copies of the Test buffer content)
	//ParseTestFileBufferData(bufParser, EndianMode::LITTLE); // First, the test content in little-endian format
	ParseTestFileBufferData(bufParser, EndianMode::BIG); // Second, the same test content again, but in big-endian format
	//GUARANTEE_RECOVERABLE( bufParser.GetRemainingSize() == 0, "ERROR");

	g_theDevConsole->AddLine(DevConsole::SUCCESS, Stringf("...successfully read file %s\n", testFilePath));
	return false;
};

//-----------------------------------------------------------------------------------------------
bool DevConsole::Command_TestBinaryFileSave(EventArgs& args)
{
	UNUSED(args);

	const char* testFilePath = "Saves/Test.binary";
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Saving test binary file '%s'...\n", testFilePath));

	// Create the test file buffer
	std::vector<unsigned char> buffer;
	buffer.reserve(1000);
	BufferWriter bufWriter(buffer);
	//GUARANTEE_RECOVERABLE_NOREPEAT0( bufWriter.GetTotalSize() == 0 );
	//GUARANTEE_RECOVERABLE_NOREPEAT0( bufWriter.GetAppendedSize() == 0 );
	AppendTestFileBufferData(bufWriter, EndianMode::LITTLE); // First, the test content in little-endian format
	//AppendTestFileBufferData(bufWriter, EndianMode::BIG);// Second, the same test content again, but in big-endian format
	//GUARANTEE_RECOVERABLE_NOREPEAT0( bufWriter.GetAppendedSize() == TEST_BUF_SIZE ); // should be 208 Bytes total (two copies of the Test buffer content)
	//GUARANTEE_RECOVERABLE_NOREPEAT0( bufWriter.GetTotalSize() == TEST_BUF_SIZE );

	// Write to disk
	bool success = SaveBinaryFileFromBuffer(testFilePath, buffer);
	if (success)
	{
		g_theDevConsole->AddLine(DevConsole::SUCCESS, Stringf("...successfully wrote file %s\n", testFilePath));
	}
	else
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("FAILED to write file %s\n", testFilePath));
	}

	return false;
};
bool DevConsole::Command_ExecuteXML(EventArgs& args)
{
	if (!args.IsKeyNameValid("file"))
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, "ERROR: Arguments are missing or incorrect, please use file=<filePath>");
	}
	else
	{
		std::string s = args.GetValue<std::string>("file", "");
		g_theDevConsole->AddLine(DevConsole::SUCCESS, "Loaded \"" + s + "\" successfully");
		g_theDevConsole->ExecuteXmlCommandScriptFile(s);
	}

	return false;
}

bool DevConsole::Command_Test_ExecuteXML(EventArgs& args)
{
	std::string stringToPrint = args.GetValue<std::string>("p", "");
	g_theDevConsole->AddLine(DevConsole::SUCCESS, stringToPrint);
	std::string timeScale = args.GetValue<std::string>("s", "1.0");
	float timeScaleInFloat = (float)atof(timeScale.c_str());
	Clock::s_theSystemClock->SetTimeScale(timeScaleInFloat);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Time Scale is set to: " + timeScale);

	return false;
}

bool DevConsole::Command_TestFunction(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, "This is a test");
	return false;
}

bool DevConsole::Command_TestSubFunction(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has subscribed test function");
	g_theEventSystem->SubscribeEventCallbackFunction("test", DevConsole::Command_TestFunction);
	return false;
}

bool DevConsole::Command_TestUnsubFunction(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has unsubscribed test function");
	g_theEventSystem->UnsubscribeEventCallbackFunction("test", DevConsole::Command_TestFunction);
	return false;
}



bool DevConsole::Command_TestUnsubMethod(EventArgs& args)
{
	std::string empty;
	std::string idInString = args.GetValue("id", empty);
	if (idInString == empty)
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, "WRONG INPUT");
	}
	int id = atoi(idInString.c_str());
	return g_theDevConsole->Test_UnsubMemberMethod(id);
}

bool DevConsole::Test_UnsubMemberMethod(int index)
{
	if (index == 0)
	{
		g_theDevConsole->AddLine(DevConsole::ERROR, "ID 0 doesn't exist");
		return false;
	}
	// IN LIST TEST
	for (size_t i = 0; i < m_test_Foo_List.size(); i++)
	{
		if ((i == index - 1) && m_test_Foo_List[i])
		{
			g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has unsubscribed faa1 function");
			g_theEventSystem->UnsubscribeEventCallbackMemberFunction("faa1", m_test_Foo_List[index - 1], &Foo::Laugh);
			return true;
		}
	}
	return false;



	// SPECIFIC CALL OUT TEST
	//if (index == 1)
	//{
	//	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has unsubscribed faa1 function");
	//	g_theEventSystem->UnsubscribeEventCallbackMemberFunction("faa1", m_faa1, &Faa::Laugh);
	//}
	//if (index == 2)
	//{
	//	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has unsubscribed faa2 function");
	//	g_theEventSystem->UnsubscribeEventCallbackMemberFunction("faa2", m_faa2, &Faa::Laugh);
	//}
	//if (index == 3)
	//{
	//	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Has unsubscribed fee function");
	//	g_theEventSystem->UnsubscribeEventCallbackMemberFunction("fee", m_fee, &Fee::Laugh);
	//}
	// return true;

}

void DevConsole::Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect) const
{
	renderer.SetModelConstants();
	renderer.SetBlendMode(BlendMode::ALPHA);
	renderer.SetDepthStencilMode(DepthMode::DISABLED);

	std::vector<Vertex_PCU> consoleBGVerts;
	consoleBGVerts.reserve(6);
	AddVertsForAABB2D(consoleBGVerts, bounds, Rgba8::DARK);
	renderer.BindTexture(nullptr);
	renderer.DrawVertexArray((int)consoleBGVerts.size(), consoleBGVerts.data());

	float textHeight = bounds.m_maxs.y / m_config.m_numLinesVisible;
	float shadowOffset = 2.2f * fontAspect;

	std::vector<Vertex_PCU> consoleTextVerts;
	consoleTextVerts.reserve(300);
	int totalLines = static_cast<int>(m_lines.size());
	int startLineIndex = IntMax(0, totalLines - (int)m_config.m_numLinesVisible - m_scrollOffset);
	int endLineIndex = IntMin(totalLines - 1, startLineIndex + (int)m_config.m_numLinesVisible - 1);

	for (int i = endLineIndex; i >= startLineIndex; i--)
	{
		int lineFromBottom = endLineIndex - i;
		Vec2 textBoundMins = Vec2(bounds.m_mins.x, bounds.m_mins.y + textHeight + textHeight * lineFromBottom);
		Vec2 textBoundMaxs = Vec2(bounds.m_maxs.x, textBoundMins.y + textHeight);
		AABB2 textBound = AABB2(textBoundMins, textBoundMaxs);

		std::string printLine;
		if (m_showFrameAndTime)
		{
			printLine = Stringf("(Frame: %i, Timestamp: %f) %s", m_lines[i].m_frameNumber, m_lines[i].m_timestamp, m_lines[i].m_text.c_str());
		}
		else
		{
			printLine = m_lines[i].m_text;
		}

		font.AddVertsForTextInBox2D(consoleTextVerts, AABB2(textBound.m_mins.x + shadowOffset, textBound.m_mins.y, textBound.m_maxs.x + shadowOffset, textBound.m_maxs.y), textHeight, printLine, Rgba8(0, 0, 0, 200), fontAspect, Vec2(0.f, 0.5f));
		font.AddVertsForTextInBox2D(consoleTextVerts, textBound, textHeight, printLine, m_lines[i].m_color, fontAspect, Vec2(0.f, 0.5f));
	}
	renderer.BindTexture(&font.GetTexture());
	renderer.DrawVertexArray((int)consoleTextVerts.size(), consoleTextVerts.data());

	std::vector<Vertex_PCU> consoleInputVerts;
	font.AddVertsForTextInBox2D(consoleInputVerts, AABB2(0 + shadowOffset, 0, bounds.m_maxs.x + shadowOffset, textHeight), textHeight, m_inputText, Rgba8::COLOR_BLACK, fontAspect, Vec2(0.f, 0.5f));
	font.AddVertsForTextInBox2D(consoleInputVerts, AABB2(0, 0, bounds.m_maxs.x, textHeight), textHeight, m_inputText, DevConsole::INPUT_TEXT, fontAspect, Vec2(0.f, 0.5f));
	renderer.BindTexture(&font.GetTexture());
	renderer.DrawVertexArray((int)consoleInputVerts.size(), consoleInputVerts.data());

	if (m_insertionPointVisible)
	{
		std::vector<Vertex_PCU> consoleInputInsertionPointVerts;

		AABB2 insertionBox;
		float insertionBoxWidth = textHeight * fontAspect / 4.5f;
		insertionBox.m_mins.x = bounds.m_mins.x + m_insertionPointPosition * textHeight * fontAspect;
		insertionBox.m_mins.y = bounds.m_mins.y;
		insertionBox.m_maxs.x = insertionBox.m_mins.x + insertionBoxWidth;
		insertionBox.m_maxs.y = insertionBox.m_mins.y + textHeight;

		if (insertionBox.m_mins.x >= bounds.m_maxs.x)
		{
			insertionBox.m_maxs.x = bounds.m_maxs.x;
			insertionBox.m_mins.x = insertionBox.m_maxs.x - insertionBoxWidth;
		}

		AddVertsForAABB2D(consoleInputInsertionPointVerts, insertionBox, DevConsole::INPUT_INSERTION_POINT);

		renderer.BindTexture(nullptr);
		renderer.DrawVertexArray((int)consoleInputInsertionPointVerts.size(), consoleInputInsertionPointVerts.data());
	}

}

DevConsole::DevConsole(DevConsoleConfig const& config)
	:m_config(config)
{
}

DevConsole::~DevConsole()
{
}

void DevConsole::Startup()
{
	if (m_config.m_renderer != nullptr)
	{
		m_font = m_config.m_renderer->CreateOrGetBitmapFont(m_config.m_fontFilePath.c_str());
	}
	m_camera = m_config.m_camera;
	g_theEventSystem->SubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);
	g_theEventSystem->SubscribeEventCallbackFunction("KeyPressed", DevConsole::Event_KeyPressed);
	g_theEventSystem->SubscribeEventCallbackFunction("help", DevConsole::Command_Help);
	g_theEventSystem->SubscribeEventCallbackFunction("clear", DevConsole::Command_Clear);
	g_theEventSystem->SubscribeEventCallbackFunction("echo", DevConsole::Command_Echo);
	g_theEventSystem->SubscribeEventCallbackFunction("timescale", DevConsole::Command_SetTimeScale);
	g_theEventSystem->SubscribeEventCallbackFunction("print", DevConsole::Command_Print);
	g_theEventSystem->SubscribeEventCallbackFunction("xml", DevConsole::Command_ExecuteXML);
	g_theEventSystem->SubscribeEventCallbackFunction("xmlTest", DevConsole::Command_Test_ExecuteXML);
	g_theEventSystem->SubscribeEventCallbackFunction("subFunction", DevConsole::Command_TestSubFunction);
	g_theEventSystem->SubscribeEventCallbackFunction("unsubFunction", DevConsole::Command_TestUnsubFunction);
	g_theEventSystem->SubscribeEventCallbackFunction("unsubMethod", DevConsole::Command_TestUnsubMethod);
	g_theEventSystem->SubscribeEventCallbackFunction("loadfile", DevConsole::Command_TestBinaryFileLoad);
	g_theEventSystem->SubscribeEventCallbackFunction("savefile", DevConsole::Command_TestBinaryFileSave);
	m_showFrameAndTime = false;
	m_insertionPointBlinkTimer = new Timer(0.5f, Clock::s_theSystemClock);
	m_insertionPointBlinkTimer->Start();

	m_faa1 = new Faa();
	m_faa1->m_ID = 1;
	m_faa2 = new Faa();
	m_faa2->m_ID = 2;
	m_fee = new Fee();

	//g_theEventSystem->SubscribeEventCallbackMemberFunction("faa1", m_faa1, &Faa::Laugh);
	//g_theEventSystem->SubscribeEventCallbackMemberFunction("faa2",m_faa2, &Faa::Laugh);
	//g_theEventSystem->SubscribeEventCallbackMemberFunction("fee", m_fee, &Fee::Laugh);

	m_test_Foo_List.push_back(m_faa1);
	m_test_Foo_List.push_back(m_faa2);
	m_test_Foo_List.push_back(m_fee);

	g_theEventSystem->SubscribeEventCallbackMemberFunction("faa1", (Foo*)m_faa1, &Foo::Laugh);
	g_theEventSystem->SubscribeEventCallbackMemberFunction("faa2", (Foo*)m_faa2, &Foo::Laugh);
	g_theEventSystem->SubscribeEventCallbackMemberFunction("fee", (Foo*)m_fee, &Foo::Laugh);


}

void DevConsole::BeginFrame()
{
	if (m_insertionPointBlinkTimer->DecrementPeriodIfElapsed())
	{
		m_insertionPointVisible = !m_insertionPointVisible;
	}
	m_frameNumber += 1;
	if (g_theInput->WasMouseWheelScrolledUp())
	{
		if (m_lines.size() < m_config.m_numLinesVisible) return;
		m_scrollOffset++;
	}
	if (g_theInput->WasMouseWheelScrolledDown())
	{
		if (m_lines.size() < m_config.m_numLinesVisible) return;
		m_scrollOffset--;
		if (m_scrollOffset < 0) m_scrollOffset = 0;
	}
}

void DevConsole::EndFrame()
{
}

void DevConsole::Shutdown()
{

}

void DevConsole::Execute(std::string const& consoleCommandText, bool echoCommand)
{
	EventArgs args;

	Strings lineList = SplitStringOnDelimiter(consoleCommandText, '\n');
	for (const auto& lineIndex : lineList)
	{
		Strings pairList = SplitStringWithQuotes(lineIndex, ' ');
		for (int pairIndex = 1; pairIndex < (int)pairList.size(); pairIndex++)
		{
			Strings pairElements = SplitStringWithQuotes(pairList[pairIndex], '=');
			if (pairElements.size() > 1)
			{
				std::string value = pairElements[1];
				TrimString(value, '\"');
				args.SetValue(pairElements[0], value);
			}
		}

		FireEvent(pairList[0], args);

		if (echoCommand)
		{
			g_theDevConsole->AddLine(DevConsole::COMMAND_ECHO, consoleCommandText);
		}
	}

	m_scrollOffset = 0;
}

void DevConsole::ExecuteXmlCommandScriptNode(XmlElement const& commandScriptXmlElement)
{
	const XmlElement* element = commandScriptXmlElement.FirstChildElement();

	while (element)
	{
		std::string command = element->Name();
		const XmlAttribute* a = element->FirstAttribute();
		while (a)
		{
			std::string value = ParseXmlAttribute(*element, a->Name(), a->Value());
			std::string name = a->Name();
			command += (" " + name + "=" + "\"" + value + "\"");

			a = a->Next();
		}
		Execute(command, false);
		element = element->NextSiblingElement();
	}
}

void DevConsole::ExecuteXmlCommandScriptFile(std::string& commandScriptXmlFilePathName)
{
	XmlDocument file;
	XmlError result = file.LoadFile(commandScriptXmlFilePathName.c_str());
	GUARANTEE_RECOVERABLE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	if (rootElement)
	{
		g_theDevConsole->ExecuteXmlCommandScriptNode(*rootElement);
	}
	else
	{
		GUARANTEE_RECOVERABLE(rootElement, "Root Element is null");
	}
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
	DevConsoleLine newLine;
	newLine.m_color = color;
	newLine.m_text = "> " + text;
	newLine.m_frameNumber = m_frameNumber;
	newLine.m_timestamp = GetCurrentTimeSeconds();
	m_lines.push_back(newLine);
}

void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride) const
{
	if (!m_isOpen)
	{
		return;
	}
	m_camera->SetOrthographicView(bounds.m_mins, bounds.m_maxs);
	rendererOverride->BeginCamera(*m_camera);
	float aspect = 1.f / Window::GetMainWindowInstance()->GetAspect();
	Render_OpenFull(bounds, *rendererOverride, *m_font, aspect);
	rendererOverride->EndCamera(*m_camera);
}

int DevConsole::GetCurrentFrame()
{
	int frame = m_frameNumber;
	return frame;
}

void DevConsole::ToggleOpen()
{
	m_isOpen = !m_isOpen;
}

bool DevConsole::IsOpen()
{
	bool open = m_isOpen;
	return open;
}

bool Faa::Laugh(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, Stringf("ID: %i laugh Hahahahaha", m_ID));
	return true;
}

bool Fee::Laugh(EventArgs& args)
{
	UNUSED(args);
	g_theDevConsole->AddLine(DevConsole::SUCCESS, "Hehehehehe");
	return true;
}


void AppendTestFileBufferData(BufferWriter& bufferWriter, EndianMode endianMode)
{
	bufferWriter.SetPreferredEndianMode(endianMode);
	bufferWriter.AppendChar('T');
	bufferWriter.AppendChar('E');
	bufferWriter.AppendChar('S');
	bufferWriter.AppendChar('T');
	bufferWriter.AppendByte(2); // Version 2
	bufferWriter.AppendByte((unsigned char)bufferWriter.GetPreferredEndianMode());
	bufferWriter.AppendBool(false);
	bufferWriter.AppendBool(true);
	bufferWriter.AppendUint32(0x12345678);
	bufferWriter.AppendInt32(-7); // signed 32-bit int
	bufferWriter.AppendFloat(1.f); // in memory looks like hex: 00 00 80 3F (or 3F 80 00 00 in big endian)
	bufferWriter.AppendDouble(3.1415926535897932384626433832795); // actually 3.1415926535897931 (best it can do)
	bufferWriter.AppendZeroTerminatedString("Hello"); // written with a trailing 0 ('\0') after (6 bytes total)
	bufferWriter.AppendStringAfter32BitLength("Is this thing on?"); // uint 17, then 17 chars (no zero-terminator after)
	bufferWriter.AppendRgba8(Rgba8(200, 100, 50, 255)); // four bytes in RGBA order (endian-independent)
	bufferWriter.AppendByte(8); // 0x08 == 8 (byte)
	bufferWriter.AppendRgb8(Rgba8(238, 221, 204, 255)); // written as 3 bytes (RGB) only; ignores Alpha
	bufferWriter.AppendByte(9); // 0x09 == 9 (byte)
	bufferWriter.AppendIntVec2(IntVec2(1920, 1080));
	bufferWriter.AppendVec2(Vec2(-0.6f, 0.8f));
	bufferWriter.AppendVertexPCU(Vertex_PCU(Vec3(3.f, 4.f, 5.f), Rgba8(100, 101, 102, 103), Vec2(0.125f, 0.625f)));
};

//------------------------------------------------------------------------------------------------
void ParseTestFileBufferData(BufferParser& bufferParser, EndianMode endianMode)
{
	// Parse known test file elements
	bufferParser.SetBufferEndianMode(endianMode);
	char fourCC0_T = bufferParser.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	char fourCC1_E = bufferParser.ParseChar(); // 'E' == 0x45 hex == 84 decimal
	char fourCC2_S = bufferParser.ParseChar(); // 'S' == 0x53 hex == 69 decimal
	char fourCC3_T = bufferParser.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	unsigned char version = bufferParser.ParseByte(); // version 2
	bufferParser.ParseByte();
	bool shouldBeFalse = bufferParser.ParseBool(); // written in buffer as byte 0 or 1
	bool shouldBeTrue = bufferParser.ParseBool(); // written in buffer as byte 0 or 1
	unsigned int largeUint = bufferParser.ParseUint32(); // 0x12345678
	int negativeSeven = bufferParser.ParseInt32(); // -7 (as signed 32-bit int)
	float oneF = bufferParser.ParseFloat(); // 1.0f
	double pi = bufferParser.ParseDouble(); // 3.1415926535897932384626433832795 (or as best it can)

	std::string helloString, isThisThingOnString;
	bufferParser.ParseZeroTerminatedString(helloString); // written with a trailing 0 ('\0') after (6 bytes total)
	bufferParser.ParseStringAfter32BitLength(isThisThingOnString); // written as uint 17, then 17 characters (no zero-terminator after)

	Rgba8 rustColor = bufferParser.ParseRgba8(); // Rgba( 200, 100, 50, 255 )
	unsigned char eight = bufferParser.ParseByte(); // 0x08 == 8 (byte)
	Rgba8 seashellColor = bufferParser.ParseRgb8(); // Rgba(238,221,204) written as 3 bytes (RGB) only; ignores Alpha
	unsigned char nine = bufferParser.ParseByte(); // 0x09 == 9 (byte)
	IntVec2 highDefRes = bufferParser.ParseIntVec2(); // IntVector2( 1920, 1080 )
	Vec2 normal2D = bufferParser.ParseVec2(); // Vector2( -0.6f, 0.8f )
	Vertex_PCU vertex = bufferParser.ParseVertexPCU(); // VertexPCU( 3.f, 4.f, 5.f, Rgba(100,101,102,103), 0.125f, 0.625f ) );

	// Validate actual values parsed
	GUARANTEE_RECOVERABLE(fourCC0_T == 'T', "ERROR");
	GUARANTEE_RECOVERABLE(fourCC1_E == 'E', "ERROR");
	GUARANTEE_RECOVERABLE(fourCC2_S == 'S', "ERROR");
	GUARANTEE_RECOVERABLE(fourCC3_T == 'T', "ERROR");
	GUARANTEE_RECOVERABLE(version == 2, "ERROR");
	GUARANTEE_RECOVERABLE(shouldBeFalse == false, "ERROR");
	GUARANTEE_RECOVERABLE(shouldBeTrue == true, "ERROR");
	GUARANTEE_RECOVERABLE(largeUint == 0x12345678, "ERROR");
	GUARANTEE_RECOVERABLE(negativeSeven == -7, "ERROR");
	GUARANTEE_RECOVERABLE(oneF == 1.f, "ERROR");
	GUARANTEE_RECOVERABLE(pi == 3.1415926535897931, "ERROR");
	GUARANTEE_RECOVERABLE(helloString == "Hello", "ERROR");
	GUARANTEE_RECOVERABLE(isThisThingOnString == "Is this thing on?", "ERROR");
	GUARANTEE_RECOVERABLE(rustColor == Rgba8(200, 100, 50, 255), "ERROR");
	GUARANTEE_RECOVERABLE(eight == 8, "ERROR");
	GUARANTEE_RECOVERABLE(seashellColor == Rgba8(238, 221, 204, 255), "ERROR");
	GUARANTEE_RECOVERABLE(nine == 9, "ERROR");
	GUARANTEE_RECOVERABLE(highDefRes == IntVec2(1920, 1080), "ERROR");
	GUARANTEE_RECOVERABLE(normal2D == Vec2(-0.6f, 0.8f), "ERROR");
	GUARANTEE_RECOVERABLE(vertex.m_position == Vec3(3.f, 4.f, 5.f), "ERROR");
	GUARANTEE_RECOVERABLE(vertex.m_color == Rgba8(100, 101, 102, 103), "ERROR");
	GUARANTEE_RECOVERABLE(vertex.m_uvTexCoords == Vec2(0.125f, 0.625f), "ERROR");
};