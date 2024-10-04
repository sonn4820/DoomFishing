#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Game/FishAIController.hpp"
#include "Game/Weapon.hpp"

std::vector<MapDefinition*> MapDefinition::s_mapDefs;

MapDefinition::MapDefinition(XmlElement& element)
	:m_name(ParseXmlAttribute(element, "name", ""))
{
	std::string imagePath = ParseXmlAttribute(element, "image", "Data/Maps/TestMap.png");
	m_image = Image(imagePath.c_str());
	ShaderConfig shaderConfig;
	shaderConfig.m_name = ParseXmlAttribute(element, "shader", "Default");
	m_shader = new Shader(shaderConfig);

	m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "spriteSheetTexture", "Data/Images/Terrain_8x8.png").c_str());
	m_skyboxTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(element, "skyboxTexture", "Data/Images/skybox.png").c_str());
	m_spriteSheetCellCount = ParseXmlAttribute(element, "spriteSheetCellCount", IntVec2(8, 8));
}

void MapDefinition::SetSpawnInfo(XmlElement& element)
{
	ActorDefinition* newActorDef = ActorDefinition::GetByName(ParseXmlAttribute(element, "actor", ""));
	m_actorDefs.push_back(newActorDef);
	m_actorPosition.push_back(ParseXmlAttribute(element, "position", Vec3()));
	m_actorOrientation.push_back(ParseXmlAttribute(element, "orientation", EulerAngles()));
	m_actorVelocity.push_back(ParseXmlAttribute(element, "velocity", Vec3()));
}

void MapDefinition::InitializeMapDefs(char const* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Root Element is null");


	XmlElement* mapDefElement = rootElement->FirstChildElement();

	while (mapDefElement)
	{
		std::string mapDefName = mapDefElement->Name();
		GUARANTEE_OR_DIE(mapDefName == "MapDefinition", "Root child element is in the wrong format");
		MapDefinition* newMapDef = new MapDefinition(*mapDefElement);

		XmlElement* mapElements = mapDefElement->FirstChildElement();
		while (mapElements)
		{
			std::string spawnInfosName = mapElements->Name();
			if (spawnInfosName == "SpawnInfos")
			{
				XmlElement* spawnElement = mapElements->FirstChildElement();
				{
					while (spawnElement)
					{
						newMapDef->SetSpawnInfo(*spawnElement);
						spawnElement = spawnElement->NextSiblingElement();
					}
				}
			}
			mapElements = mapElements->NextSiblingElement();
		}
		s_mapDefs.push_back(newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

void MapDefinition::ClearDefinition()
{
	for (size_t i = 0; i < s_mapDefs.size(); i++)
	{
		if (s_mapDefs[i] != nullptr)
		{
			delete s_mapDefs[i];
			s_mapDefs[i] = nullptr;
		}
	}
}

MapDefinition* MapDefinition::GetByName(std::string const& name)
{
	for (size_t i = 0; i < s_mapDefs.size(); i++)
	{
		if (s_mapDefs[i]->m_name == name)
		{
			return s_mapDefs[i];
		}
	}
	return nullptr;
}

Map::Map(MapDefinition* mapDef)
	:m_mapDef(mapDef)
{

	m_dimensions = mapDef->m_image.GetDimensions();
	m_allActorsList.reserve(50);
}

void Map::StartUp()
{
	m_texture = m_mapDef->m_spriteSheetTexture;
	m_shader = g_theRenderer->CreateShader(m_mapDef->m_shader->GetName().c_str(), VertexType::Vertex_PCUTBN);

	PopulateTileMap();

	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN) * (unsigned int)m_vertexes.size());
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int) * (unsigned int)m_indexes.size());
	g_theRenderer->CopyCPUToGPU(m_vertexes.data(), (int)(m_vertexes.size() * sizeof(Vertex_PCUTBN)), m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(m_indexes.data(), (int)(m_indexes.size() * sizeof(unsigned int)), m_indexBuffer);

	AddSkyBoxVerts(m_skyboxVertexes);

	PopulateActors();

	g_theAudio->SetNumListeners(g_theGame->m_numPlayer);

	m_fishSpawnTimer = new Timer(7.f, g_theGame->m_clock);
	m_fishSpawnTimer->Start();
}

void Map::Update()
{
	HandleInput();

	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i])
		{
			m_allActorsList[i]->Update();
		}
	}
	SpawnFishIfTotalFishLessThan(30);
	CollideActors();
	CollideActorsVsMap();

	DeleteAllGarbageActors();

	for (int i = 0; i < g_theGame->m_numPlayer; i++)
	{
		if (!GetActorByUID(m_playerUID[i]))
		{
			SpawnPlayer(i);
		}
	}
}

void Map::Render(int playerID) const
{
	g_theRenderer->BeginCamera(*g_theGame->m_player[playerID]->GetCamera());

	RenderTile();
	RenderSkybox();

	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i])
		{
			m_allActorsList[i]->Render(g_theGame->m_player[playerID]->GetCamera());
			if (m_allActorsList[i]->m_currentWeapon)
			{
				m_allActorsList[i]->m_currentWeapon->Render(*g_theGame->m_player[playerID]->GetCamera());
			}
		}
	}

	g_theRenderer->EndCamera(*g_theGame->m_player[playerID]->GetCamera());
}

void Map::Shutdown()
{
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i] != nullptr)
		{
			delete m_allActorsList[i];
			m_allActorsList[i] = nullptr;
		}
	}
	m_allActorsList.clear();

	m_texture = nullptr;
	m_shader = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

void Map::HandleInput()
{

}

void Map::PopulateTileMap()
{
	// Define tiles
	int size = m_dimensions.x * m_dimensions.y;
	m_tiles.reserve(size);

	for (int y = 0; y < m_dimensions.y; y++)
	{
		for (int x = 0; x < m_dimensions.x; x++)
		{
			Tile* newTile = new Tile(IntVec2(x, y));
			Rgba8 colorDef = m_mapDef->m_image.GetTexelColor(IntVec2(x, y));
			for (size_t i = 0; i < TileDefinition::s_tileDefs.size(); i++)
			{
				if (TileDefinition::s_tileDefs[i]->m_mapImagePixelColor == colorDef)
				{
					newTile->m_tileDef = TileDefinition::s_tileDefs[i];
					break;
				}
			}
			newTile->m_bounds = AABB3(Vec3((float)x, (float)y, 0.f), Vec3((float)x + 1.f, (float)y + 1.f, 1.f));
			m_tiles.push_back(newTile);
		}
	}

	// Add vertexes for tiles
	int tileVertsSize = (int)m_tiles.size() * 4 * 6;
	int tileIndexesSize = (int)m_tiles.size() * 6 * 6;
	m_vertexes.reserve(tileVertsSize);
	m_indexes.reserve(tileIndexesSize);
	for (int i = 0; i < (int)m_tiles.size(); i++)
	{
		AddVertsForTile(m_vertexes, m_indexes, i);
	}
}

void Map::AddVertsForTile(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, int tileCoord) const
{
	Tile const& tile = *m_tiles[tileCoord];
	AABB3 bounds = tile.GetBound();
	Rgba8 color = tile.m_tileDef->m_mapImagePixelColor;
	IntVec2 floorSpriteCoord = tile.m_tileDef->m_floorSpriteCoord;
	IntVec2 ceilingSpriteCoord = tile.m_tileDef->m_ceilingSpriteCoord;
	IntVec2 wallSpriteCoord = tile.m_tileDef->m_wallSpriteCoord;

	AABB2 floorUvBox = g_theGame->m_terrainSprites->GetSpriteUVs(g_theGame->m_terrainSprites->GetSpriteIndex(tile.m_tileDef->m_floorSpriteCoord));
	AABB2 ceilingUvBox = g_theGame->m_terrainSprites->GetSpriteUVs(g_theGame->m_terrainSprites->GetSpriteIndex(tile.m_tileDef->m_ceilingSpriteCoord));
	AABB2 wallUvBox = g_theGame->m_terrainSprites->GetSpriteUVs(g_theGame->m_terrainSprites->GetSpriteIndex(tile.m_tileDef->m_wallSpriteCoord));

	float boundZMin = (tile.m_tileDef->m_isWater) ? bounds.m_mins.z - 0.187f : bounds.m_mins.z;
	float boundZMax = (tile.m_tileDef->m_isWater) ? bounds.m_maxs.z - 0.187f : bounds.m_maxs.z;

	float offSet = (tile.m_tileDef->m_isRenderOffset) ? tile.m_tileDef->m_renderOffset : 0.f;

	if (tile.m_tileDef->m_isBridge)
	{
		float offsetBridge = 0.f;

		if (tile.m_tileDef->m_hasSideFaces)
		{
			if (tile.m_tileDef->m_hasSouthFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, -boundZMin), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, -boundZMin), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasNorthFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, -boundZMin), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, -boundZMin), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasWestFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin + offsetBridge), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin + offsetBridge), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasEastFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin + offsetBridge), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin + offsetBridge), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}
		}


		AABB2 bridgeEndUvBox = g_theGame->m_terrainSprites->GetSpriteUVs(g_theGame->m_terrainSprites->GetSpriteIndex(tile.m_tileDef->m_waterGroundSpriteCoords));

		if (tile.m_tileDef->m_bridgeWestEnd)
		{
			AddVertsForQuad3DInverseNormal(verts, indexes,
				Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin - 0.187f), //BL
				Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin - 0.187f), //BR
				Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax - 0.187f), //TL
				Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax - 0.187f), //TR
				Rgba8::COLOR_WHITE, bridgeEndUvBox);
		}

		if (tile.m_tileDef->m_bridgeEastEnd)
		{
			AddVertsForQuad3DInverseNormal(verts, indexes,
				Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin - 0.187f), //BL
				Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin - 0.187f), //BR
				Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax - 0.187f), //TL
				Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax - 0.187f), //TR
				Rgba8::COLOR_WHITE, bridgeEndUvBox);
		}

		if (tile.m_tileDef->m_bridgeSouthEnd)
		{
			AddVertsForQuad3DInverseNormal(verts, indexes,
				Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin - 0.187f), //BL
				Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin - 0.187f), //BR
				Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax - 0.187f), //TL
				Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax - 0.187f), //TR
				Rgba8::COLOR_WHITE, bridgeEndUvBox);
		}

		if (tile.m_tileDef->m_bridgeNorthEnd)
		{
			AddVertsForQuad3DInverseNormal(verts, indexes,
				Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin - 0.187f), //BL
				Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin - 0.187f), //BR
				Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax - 0.187f), //TL
				Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax - 0.187f), //TR
				Rgba8::COLOR_WHITE, bridgeEndUvBox);
		}

		AABB2 waterBridgeUvBox = g_theGame->m_terrainSprites->GetSpriteUVs(g_theGame->m_terrainSprites->GetSpriteIndex(tile.m_tileDef->m_waterBridgeSpriteCoords));

		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_mins.x, bounds.m_mins.y, boundZMin + offsetBridge), //BL
			Vec3(bounds.m_maxs.x, bounds.m_mins.y, boundZMin + offsetBridge), //BR
			Vec3(bounds.m_mins.x, bounds.m_maxs.y, boundZMin + offsetBridge), //TL
			Vec3(bounds.m_maxs.x, bounds.m_maxs.y, boundZMin + offsetBridge), //TR
			Rgba8::COLOR_WHITE, floorUvBox);

		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_mins.x, bounds.m_mins.y, boundZMin - 0.187f), //BL
			Vec3(bounds.m_maxs.x, bounds.m_mins.y, boundZMin - 0.187f), //BR
			Vec3(bounds.m_mins.x, bounds.m_maxs.y, boundZMin - 0.187f), //TL
			Vec3(bounds.m_maxs.x, bounds.m_maxs.y, boundZMin - 0.187f), //TR
			Rgba8::COLOR_WHITE, waterBridgeUvBox);


		return;
	}
	if (tile.m_tileDef->m_isSolid)
	{
		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BL
			Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BR
			Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TL
			Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TR
			Rgba8::COLOR_WHITE, wallUvBox);

		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
			Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
			Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
			Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
			Rgba8::COLOR_WHITE, wallUvBox);

		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
			Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BR
			Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
			Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TR
			Rgba8::COLOR_WHITE, wallUvBox);

		AddVertsForQuad3D(verts, indexes,
			Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BL
			Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
			Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TL
			Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
			Rgba8::COLOR_WHITE, wallUvBox);
		return;
	}

	AddVertsForQuad3D(verts, indexes,
		Vec3(bounds.m_mins.x, bounds.m_mins.y, boundZMin), //BL
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, boundZMin), //BR
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, boundZMin), //TL
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, boundZMin), //TR
		Rgba8::COLOR_WHITE, floorUvBox);

	AddVertsForQuad3D(verts, indexes,
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, boundZMax), //BL
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, boundZMax), //BR
		Vec3(bounds.m_mins.x, bounds.m_mins.y, boundZMax), //TL
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, boundZMax), //TR
		Rgba8::COLOR_WHITE, ceilingUvBox);

	if (tile.m_tileDef->m_hasSideFaces)
	{
		if (tile.m_tileDef->m_isRenderLit)
		{
			if (tile.m_tileDef->m_hasWestFace)
			{
				AddVertsForQuad3DInverseNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasEastFace)
			{
				AddVertsForQuad3DInverseNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasSouthFace)
			{
				AddVertsForQuad3DInverseNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasNorthFace)
			{
				AddVertsForQuad3DInverseNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}
		}
		else
		{
			if (tile.m_tileDef->m_hasWestFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasEastFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasSouthFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMin), //BL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMin), //BR
					Vec3(bounds.m_mins.x + offSet, bounds.m_mins.y + offSet, boundZMax), //TL
					Vec3(bounds.m_maxs.x - offSet, bounds.m_mins.y + offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}

			if (tile.m_tileDef->m_hasNorthFace)
			{
				AddVertsForQuad3DNoNormal(verts, indexes,
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMin), //BL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMin), //BR
					Vec3(bounds.m_maxs.x - offSet, bounds.m_maxs.y - offSet, boundZMax), //TL
					Vec3(bounds.m_mins.x + offSet, bounds.m_maxs.y - offSet, boundZMax), //TR
					Rgba8::COLOR_WHITE, wallUvBox);
			}
		}
	}
}


void Map::SetTileType(int index, Rgba8 pixelColor)
{
	for (size_t i = 0; i < TileDefinition::s_tileDefs.size(); i++)
	{
		if (TileDefinition::s_tileDefs[i]->m_mapImagePixelColor == pixelColor)
		{
			m_tiles[index]->m_tileDef = TileDefinition::s_tileDefs[i];
			break;
		}
	}
}

void Map::SetTileType(int x, int y, Rgba8 pixelColor)
{
	int index = GetTileIndexesFromCoord(x, y);
	SetTileType(index, pixelColor);
}

int Map::GetTileIndexesFromCoord(int x, int y) const
{
	return  x + y * m_dimensions.x;
}
int Map::GetTileIndexesFromCoord(IntVec2 coord) const
{
	return coord.x + coord.y * m_dimensions.x;
}
//..............................
int Map::GetTileXFromIndexes(int index) const
{
	return index % m_dimensions.x;
}
//..............................
int Map::GetTileYFromIndexes(int index) const
{
	return index / m_dimensions.y;
}

void Map::RenderTile() const
{
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(g_theGame->m_spriteSheetTexture);
	g_theRenderer->BindShader(m_shader, VertexType::Vertex_PCUTBN);
	g_theRenderer->SetLightConstants(Vec3(m_sunX, m_sunY, -1.f), m_sunIntensity, m_ambIntensity);
	g_theRenderer->DrawIndexedBuffer(m_vertexBuffer, m_indexBuffer, (int)m_indexes.size(), 0, VertexType::Vertex_PCUTBN);
}

bool Map::IsTileSolid(IntVec2 coord) const
{
	if (!IsTileInBound(coord))
	{
		return true;
	}
	return GetTile(coord)->m_tileDef->m_isSolid;
}

bool Map::IsTileWater(IntVec2 coord) const
{
	if (!IsTileInBound(coord))
	{
		return false;
	}
	return GetTile(coord)->m_tileDef->m_isWater;
}

bool Map::IsTileBridge(IntVec2 coord) const
{
	if (!IsTileInBound(coord))
	{
		return false;
	}
	return GetTile(coord)->m_tileDef->m_isBridge;
}

bool Map::IsTileInBound(IntVec2 coord) const
{
	return coord.x >= 0 && coord.y >= 0 && coord.x < m_dimensions.x && coord.y < m_dimensions.y;
}

bool Map::IsTileInBoundFromPos(Vec3 pos) const
{
	int x = RoundDownToInt(pos.x);
	int y = RoundDownToInt(pos.y);
	return IsTileInBound(IntVec2(x, y));
}

bool Map::IsPositionInBound(float x, float y, float tolerance)
{
	return x >= -tolerance && y >= -tolerance && x < (float)m_dimensions.x + tolerance && y < (float)m_dimensions.y + tolerance;
}

Tile* Map::GetTile(IntVec2 coord) const
{
	int index = GetTileIndexesFromCoord(coord);
	return m_tiles[index];
}

Tile* Map::GetTileFromPos(Vec3 pos) const
{
	int x = RoundDownToInt(Clamp(pos.x, 0.f, (float)m_dimensions.x - 1.f));
	int y = RoundDownToInt(Clamp(pos.y, 0.f, (float)m_dimensions.y - 1.f));
	return GetTile(IntVec2(x, y));
}

RaycastResult Map::RaycastWorldXY(Vec3 startPos, Vec3 fwdNormal, float maxDist) const
{
	RaycastResult result;
	result.m_rayStartPos = startPos;
	result.m_rayFwdNormal = fwdNormal;
	result.m_rayMaxLength = maxDist;

	IntVec2 currentTileXY = IntVec2(RoundDownToInt(startPos.x), RoundDownToInt(startPos.y));
	if (IsTileSolid(currentTileXY) && IsTileInBound(currentTileXY))
	{
		if (!(startPos.z > 1.f || startPos.z < 0.f))
		{
			result.m_didImpact = true;
			result.m_impactPos = startPos;
			result.m_impactDist = 0.f;
			result.m_impactNormal = fwdNormal * -1.f;
			return result;
		}
	}

	// X
	float fwdDistPerXCrossing = 1.f / abs(fwdNormal.x);
	int tileStepDirectionX = (fwdNormal.x > 0) ? 1 : -1;
	float xAtFirstXCrossing = currentTileXY.x + (tileStepDirectionX + 1) / 2.f;
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPos.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing;

	// Y
	float fwdDistPerYCrossing = 1.f / abs(fwdNormal.y);
	int tileStepDirectionY = (fwdNormal.y > 0) ? 1 : -1;
	float yAtFirstYCrossing = currentTileXY.y + (tileStepDirectionY + 1) / 2.f;
	float yDistToFirstYCrossing = yAtFirstYCrossing - startPos.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing;

	for (;;)
	{
		if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing)
		{
			if (fwdDistAtNextXCrossing > maxDist)
			{
				result.m_didImpact = false;
				return result;
			}
			currentTileXY.x += tileStepDirectionX;
			if (IsTileSolid(currentTileXY) && IsTileInBound(currentTileXY))
			{
				result.m_impactTileCoord = IntVec2(currentTileXY.x, currentTileXY.y);
				result.m_didImpact = true;
				result.m_impactDist = fwdDistAtNextXCrossing;
				result.m_impactPos = startPos + fwdNormal * result.m_impactDist;
				if (result.m_impactPos.z > 1.f || result.m_impactPos.z < 0)
				{
					result.m_didImpact = false;
					fwdDistAtNextXCrossing += fwdDistPerXCrossing;
					continue;
				}
				result.m_impactNormal = Vec3((float)-tileStepDirectionX, 0.f, 0.f);
				return result;
			}
			else
			{
				fwdDistAtNextXCrossing += fwdDistPerXCrossing;
			}
		}
		else
		{
			if (fwdDistAtNextYCrossing > maxDist)
			{
				result.m_didImpact = false;
				return result;
			}
			currentTileXY.y += tileStepDirectionY;
			if (IsTileSolid(currentTileXY) && IsTileInBound(currentTileXY))
			{
				result.m_impactTileCoord = IntVec2(currentTileXY.x, currentTileXY.y);
				result.m_didImpact = true;
				result.m_impactDist = fwdDistAtNextYCrossing;
				result.m_impactPos = startPos + fwdNormal * result.m_impactDist;
				if (result.m_impactPos.z > 1.f || result.m_impactPos.z < 0)
				{
					result.m_didImpact = false;
					fwdDistAtNextYCrossing += fwdDistPerYCrossing;
					continue;
				}
				result.m_impactNormal = Vec3(0.f, (float)-tileStepDirectionY, 0.f);
				return result;
			}
			else
			{
				fwdDistAtNextYCrossing += fwdDistPerYCrossing;
			}
		}
	}
}

RaycastResult Map::RaycastWorldZ(Vec3 startPos, Vec3 fwdNormal, float maxDist) const
{
	// RECORD IF IT HITS CEILING OR FLOOR]
	RaycastResult ZCheck;
	ZCheck.m_rayStartPos = startPos;
	ZCheck.m_rayFwdNormal = fwdNormal;
	ZCheck.m_rayMaxLength = maxDist;
	Vec3 endPos = startPos + fwdNormal * maxDist;

	if (fwdNormal.z > 0)
	{
		float t = (MAP_HEIGHT - startPos.z) / (endPos.z - startPos.z);
		if (t >= 0.f && t <= MAP_HEIGHT)
		{
			ZCheck.m_didImpact = true;
			ZCheck.m_impactDist = maxDist * t;
			ZCheck.m_impactPos = startPos + fwdNormal * ZCheck.m_impactDist;
			if (ZCheck.m_impactPos.x < 0 || ZCheck.m_impactPos.x > m_dimensions.x - 1 || ZCheck.m_impactPos.y < 0 || ZCheck.m_impactPos.y > m_dimensions.y - 1)
			{
				ZCheck.m_didImpact = false;
				return ZCheck;
			}
			ZCheck.m_impactTileCoord = IntVec2((int)ZCheck.m_impactPos.x, (int)ZCheck.m_impactPos.y);
			ZCheck.m_impactNormal = Vec3(0.f, 0.f, -1.f);
			return ZCheck;
		}
	}
	else
	{
		float t = (-startPos.z) / (endPos.z - startPos.z);
		if (t >= 0.f && t <= MAP_HEIGHT)
		{
			ZCheck.m_didImpact = true;
			ZCheck.m_impactDist = maxDist * t;
			ZCheck.m_impactPos = startPos + fwdNormal * ZCheck.m_impactDist;
			if (ZCheck.m_impactPos.x < 0 || ZCheck.m_impactPos.x > m_dimensions.x - 1 || ZCheck.m_impactPos.y < 0 || ZCheck.m_impactPos.y > m_dimensions.y - 1)
			{
				ZCheck.m_didImpact = false;
				return ZCheck;
			}
			ZCheck.m_impactTileCoord = IntVec2((int)ZCheck.m_impactPos.x, (int)ZCheck.m_impactPos.y);
			ZCheck.m_impactNormal = Vec3(0.f, 0.f, 1.f);
			return ZCheck;
		}
	}

	ZCheck.m_didImpact = false;
	return ZCheck;
}

RaycastResult Map::RaycastActors(Vec3 startPos, Vec3 fwdNormal, float maxDist, ActorUID ownerActor) const
{
	RaycastResult closestActor;
	closestActor.m_rayStartPos = startPos;
	closestActor.m_rayFwdNormal = fwdNormal;
	closestActor.m_rayMaxLength = maxDist;

	closestActor.m_impactDist = 9999.f;
	for (size_t i = 0; i < g_currentMap->m_allActorsList.size(); i++)
	{
		if (!g_currentMap->m_allActorsList[i] || !ownerActor.IsValid())
		{
			continue;
		}
		if (!g_currentMap->m_allActorsList[i]->m_actorDef->m_visible || g_currentMap->m_allActorsList[i]->m_isDead)
		{
			continue;
		}
		if (g_currentMap->m_allActorsList[i] == g_currentMap->GetActorByUID(ownerActor))
		{
			continue;
		}

		Vec2 centerXY = g_currentMap->m_allActorsList[i]->GetColliderCenterXY();
		FloatRange minmaxZ = g_currentMap->m_allActorsList[i]->GetColliderMinMaxZ();
		float radius = g_currentMap->m_allActorsList[i]->m_actorDef->m_physiscRadius;
		RaycastResult3D resultVsActor = RaycastVsZCynlinder3D(startPos, fwdNormal, maxDist, centerXY, minmaxZ, radius);
		if (resultVsActor.m_didImpact)
		{
			if (resultVsActor.m_impactDist < closestActor.m_impactDist)
			{
				closestActor.m_didImpact = resultVsActor.m_didImpact;
				closestActor.m_impactDist = resultVsActor.m_impactDist;
				closestActor.m_impactPos = resultVsActor.m_impactPos;
				closestActor.m_impactNormal = resultVsActor.m_impactNormal;
				closestActor.m_targetUID = g_currentMap->m_allActorsList[i]->m_UID;
			}
		}
	}
	return closestActor;
}

RaycastResult Map::RaycastAll(Vec3 startPos, Vec3 fwdNormal, float maxDist, ActorUID ownerActor) const
{
	RaycastResult result;
	RaycastResult resultVsWorldXY = RaycastWorldXY(startPos, fwdNormal, maxDist);
	RaycastResult resultVsWorldZ = RaycastWorldZ(startPos, fwdNormal, maxDist);
	RaycastResult resultVsActors = RaycastActors(startPos, fwdNormal, maxDist, ownerActor);

	if (resultVsWorldXY.m_didImpact)
	{
		result = resultVsWorldXY;
	}
	if (resultVsWorldZ.m_didImpact)
	{
		if (result.m_didImpact)
		{
			if (resultVsWorldZ.m_impactDist < result.m_impactDist)
			{
				result = resultVsWorldZ;
			}
		}
		else
		{
			result = resultVsWorldZ;
		}
	}
	if (resultVsActors.m_didImpact)
	{
		if (result.m_didImpact)
		{
			if (resultVsActors.m_impactDist < result.m_impactDist)
			{
				result = resultVsActors;
			}
		}
		else
		{
			result = resultVsActors;
		}
	}
	return result;
}

ActorUID Map::SpawnActor(std::string actorDefName, Vec3 position, EulerAngles orientation, Rgba8 solidColor, Rgba8 wireFrameColor, ActorUID owner)
{
	Actor* newActor = nullptr;
	int nullIndex = -1;
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i] == nullptr)
		{
			nullIndex = (int)i;
			break;
		}
	}

	if (m_allActorsList.size() >= 0x0000fffe && nullIndex == -1)
	{
		ERROR_AND_DIE("Reach size actor limit");
	}

	GenerateUID();
	ActorDefinition* actorDef = ActorDefinition::GetByName(actorDefName);
	int uid = AddActorToList(newActor, m_allActorsList);
	m_allActorsList[uid] = new Actor(actorDef, this, m_actorSalt, uid);
	m_allActorsList[uid]->m_position = position;
	m_allActorsList[uid]->m_orientationDegrees = orientation;
	m_allActorsList[uid]->m_actorDef->m_solidColor = solidColor;
	m_allActorsList[uid]->m_actorDef->m_wireframeColor = wireFrameColor;
	m_allActorsList[uid]->m_owner = owner;
	return m_allActorsList[uid]->m_UID;
}

void Map::SpawnPlayer(int playerID)
{
	std::vector<size_t> spawnPointIndexes;
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (!m_allActorsList[i])
		{
			continue;
		}
		if (m_allActorsList[i]->m_actorDef->m_name == "SpawnPoint")
		{
			spawnPointIndexes.push_back(i);
		}
	}

	size_t randomSpawnPointIndex = 0;

	for (;;)
	{
		if (playerID == 1)
		{
			randomSpawnPointIndex = g_theRNG->RollRandomIntLessThan((int)spawnPointIndexes.size());
			if (m_allActorsList[randomSpawnPointIndex]->m_position == m_playerUID[0]->m_position)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else
		{
			randomSpawnPointIndex = g_theRNG->RollRandomIntLessThan((int)spawnPointIndexes.size());
			break;
		}
	}

	Vec3 position = m_allActorsList[randomSpawnPointIndex]->m_position;
	EulerAngles orientation = m_allActorsList[randomSpawnPointIndex]->m_orientationDegrees;
	m_playerUID[playerID] = SpawnActor("Marine", position, orientation);
	g_theGame->m_player[playerID]->Possess(m_playerUID[playerID]);

	if (g_theGame->m_player[playerID]->m_numPlayerDeath > 0)
	{
		m_playerUID[playerID]->PlaySoundByName("Respawn");
	}
	else
	{
		m_playerUID[playerID]->PlaySoundByName("FirstSpawn");
	}

}

void Map::PopulateActors()
{
	for (int x = 0; x < m_dimensions.x; x++)
	{
		for (int y = 0; y < m_dimensions.y; y++)
		{
			Tile* tile = GetTile(IntVec2(x, y));
			if (tile->m_tileDef->m_isWater)
			{
				m_fishSpawnPosList.emplace_back(tile->m_tileCoords);
			}
		}
	}

	std::vector<IntVec2> initialFishSpawnList = m_fishSpawnPosList;

	std::vector<IntVec2> demonSpawnPosList;
	for (int x = 20; x < m_dimensions.x; x++)
	{
		for (int y = 20; y < m_dimensions.y; y++)
		{
			Tile* tile = GetTile(IntVec2(x, y));
			if (!tile->m_tileDef->m_isWater && !tile->m_tileDef->m_isBridge && !tile->m_tileDef->m_isSolid)
			{
				demonSpawnPosList.emplace_back(tile->m_tileCoords);
			}
		}
	}

	for (size_t i = 0; i < m_mapDef->m_actorDefs.size(); i++)
	{
		if (m_mapDef->m_actorDefs[i]->m_faction == "Fish")
		{
			std::string name = m_mapDef->m_actorDefs[i]->m_name;
			int randIndexPos = g_theRNG->RollRandomIntInRange(0, (int)initialFishSpawnList.size() - 1);
			float yaw = g_theRNG->RollRandomFloatInRange(0.f, 360.f);

			Vec3 position = Vec3(initialFishSpawnList[randIndexPos].x + 0.5f, initialFishSpawnList[randIndexPos].y + 0.5f, 0.f);
			EulerAngles orientation = EulerAngles(yaw, 0.f, 0.f);
			ActorUID actorUID = SpawnActor(name, position, orientation);
			FishAIController* newFish = new FishAIController();
			newFish->Possess(actorUID);
			initialFishSpawnList.erase(initialFishSpawnList.begin() + randIndexPos);
			m_fishCount++;
		}
		else if (m_mapDef->m_actorDefs[i]->m_faction == "Demon" && m_mapDef->m_actorPosition[i] == Vec3())
		{
			std::string name = m_mapDef->m_actorDefs[i]->m_name;
			int randIndexPos = g_theRNG->RollRandomIntInRange(0, (int)demonSpawnPosList.size() - 1);
			float yaw = g_theRNG->RollRandomFloatInRange(0.f, 360.f);

			Vec3 position = Vec3(demonSpawnPosList[randIndexPos].x + 0.5f, demonSpawnPosList[randIndexPos].y + 0.5f, 0.f);
			EulerAngles orientation = EulerAngles(yaw, 0.f, 0.f);
			ActorUID actorUID = SpawnActor(name, position, orientation);
			AIController* newFish = new AIController();
			newFish->Possess(actorUID);
			demonSpawnPosList.erase(demonSpawnPosList.begin() + randIndexPos);
		}
		else
		{
			std::string name = m_mapDef->m_actorDefs[i]->m_name;
			Vec3 position = m_mapDef->m_actorPosition[i];
			EulerAngles orientation = m_mapDef->m_actorOrientation[i];
			ActorUID actorUID = SpawnActor(name, position, orientation);
			if (m_mapDef->m_actorDefs[i]->m_AIEnabled)
			{
				AIController* newAI = new AIController();
				newAI->Possess(actorUID);
			}
		}
	}
}

Actor* Map::GetActorByUID(ActorUID actorUID)
{
	if (!actorUID.IsValid())
	{
		return nullptr;
	}
	if (m_allActorsList[actorUID.GetIndex()] == nullptr)
	{
		return nullptr;
	}
	if (m_allActorsList[actorUID.GetIndex()]->m_UID.m_data != actorUID.m_data)
	{
		return nullptr;
	}
	return actorUID.GetActor();
}

ActorUID Map::GetClosestVisibleEnemy(ActorUID searcher, std::string enemyFaction)
{
	float closestDistance = 9999.f;
	ActorUID closestTarget = ActorUID::INVALID;
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i])
		{
			if (!m_allActorsList[i]->m_actorDef->m_visible)
			{
				continue;
			}
			if (m_allActorsList[i]->m_actorDef->m_faction == enemyFaction)
			{
				ActorUID target = m_allActorsList[i]->m_UID;
				float toTagetDistance = (searcher->m_position - target->m_position).GetLength();
				if (toTagetDistance < closestDistance)
				{
					closestTarget = target;
				}
			}
		}
	}
	return closestTarget;
}

void Map::GenerateUID()
{
	if (m_actorSalt >= s_maxSalt)
	{
		m_actorSalt = 0;
	}
	else
	{
		m_actorSalt++;
	}
}

void Map::CollideActors()
{
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		for (size_t j = i + 1; j < m_allActorsList.size(); j++)
		{
			CollideActorVsActor(m_allActorsList[i], m_allActorsList[j]);
		}
	}
}

void Map::CollideActorVsActor(Actor* actorA, Actor* actorB)
{
	if (actorA == nullptr || actorB == nullptr)
	{
		return;
	}
	if (!actorA->m_actorDef->m_visible || !actorB->m_actorDef->m_visible || actorA->m_actorDef->m_faction == "Fish" || actorB->m_actorDef->m_faction == "Fish")
	{
		return;
	}
	if (actorA->m_owner.IsValid() || actorB->m_owner.IsValid())
	{
		if (actorA->m_UID == actorB->m_owner || actorB->m_UID == actorA->m_owner || actorA->m_owner == actorB->m_owner)
		{
			return;
		}
	}

	Vec2 actorACenterXY = actorA->GetColliderCenterXY();
	FloatRange actorAMinMaxZ = actorA->GetColliderMinMaxZ();
	float actorARadius = actorA->m_actorDef->m_physiscRadius;

	Vec2 actorBCenterXY = actorB->GetColliderCenterXY();
	FloatRange actorBMinMaxZ = actorB->GetColliderMinMaxZ();
	float actorBRadius = actorB->m_actorDef->m_physiscRadius;

	if (!actorA->m_isStatic && actorA->m_actorDef->m_collideWithActors && !actorB->m_isStatic && actorB->m_actorDef->m_collideWithActors)
	{
		if (PushZCylindersOutOfEachOther3D(actorACenterXY, actorAMinMaxZ, actorARadius, actorBCenterXY, actorBMinMaxZ, actorBRadius) && !actorA->m_isDead && !actorB->m_isDead)
		{
			actorA->SetActorPosition(actorACenterXY, actorAMinMaxZ);
			actorA->OnCollide(actorB);
			actorB->SetActorPosition(actorBCenterXY, actorBMinMaxZ);
			actorB->OnCollide(actorA);
		}
	}
	else
	{
		if (!actorA->m_isStatic && actorA->m_actorDef->m_collideWithActors && actorB->m_actorDef->m_collideWithActors)
		{
			if (PushZCylindersOutOfZCylinders3D(actorACenterXY, actorAMinMaxZ, actorARadius, actorBCenterXY, actorBMinMaxZ, actorBRadius) && !actorA->m_isDead)
			{
				actorA->SetActorPosition(actorACenterXY, actorAMinMaxZ);
				actorA->OnCollide(actorB);
			}
		}
		if (!actorB->m_isStatic && actorB->m_actorDef->m_collideWithActors && actorA->m_actorDef->m_collideWithActors)
		{
			if (PushZCylindersOutOfZCylinders3D(actorBCenterXY, actorBMinMaxZ, actorBRadius, actorACenterXY, actorAMinMaxZ, actorARadius) && !actorB->m_isDead)
			{
				actorB->SetActorPosition(actorBCenterXY, actorBMinMaxZ);
				actorB->OnCollide(actorA);
			}
		}
	}

}

void Map::CollideActorsVsMap()
{
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		CollideActorVsMap(m_allActorsList[i]);
	}
}

void Map::DebugPossessNext()
{
	++m_debugPossessedIndex;
	if (m_debugPossessedIndex > (int)m_actorsCanBePossessedList.size() - 1)
	{
		m_debugPossessedIndex = 0;
	}
	if (GetActorByUID(m_actorsCanBePossessedList[m_debugPossessedIndex]))
	{
		ActorUID nextUID = m_actorsCanBePossessedList[m_debugPossessedIndex]->m_UID;
		if (!nextUID->m_isDead && nextUID.IsValid())
		{
			g_theGame->m_player[0]->Possess(m_actorsCanBePossessedList[m_debugPossessedIndex]->m_UID);
		}
	}
	else
	{
		m_actorsCanBePossessedList.erase(m_actorsCanBePossessedList.begin() + m_debugPossessedIndex);
		m_debugPossessedIndex--;
		DebugPossessNext();
	}
}

void Map::AddAllActorsCanBePossess()
{
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i]->m_actorDef->m_canBePossesed)
		{
			m_actorsCanBePossessedList.push_back(m_allActorsList[i]->m_UID);
		}
	}
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i]->m_UID == g_theGame->m_player[0]->m_controllerActorUID)
		{
			m_debugPossessedIndex = (int)i;
		}
	}
}

void Map::CollideActorVsMap(Actor* actor)
{
	if (actor == nullptr)
	{
		return;
	}
	if (actor->m_isStatic)
	{
		return;
	}
	Vec2 actorCenterXY = actor->GetColliderCenterXY();
	FloatRange actorMinMaxZ = actor->GetColliderMinMaxZ();
	float actorRadius = actor->m_actorDef->m_physiscRadius;

	if (actor->m_actorDef->m_faction != "Bait")
	{
		Tile* adjTile[8];
		adjTile[0] = GetTileFromPos(actor->m_position + Vec3(1.f, 0.f, 0.f));
		adjTile[1] = GetTileFromPos(actor->m_position + Vec3(-1.f, 0.f, 0.f));
		adjTile[2] = GetTileFromPos(actor->m_position + Vec3(0.f, 1.f, 0.f));
		adjTile[3] = GetTileFromPos(actor->m_position + Vec3(0.f, -1.f, 0.f));
		adjTile[4] = GetTileFromPos(actor->m_position + Vec3(1.f, 1.f, 0.f));
		adjTile[5] = GetTileFromPos(actor->m_position + Vec3(1.f, -1.f, 0.f));
		adjTile[6] = GetTileFromPos(actor->m_position + Vec3(-1.f, 1.f, 0.f));
		adjTile[7] = GetTileFromPos(actor->m_position + Vec3(-1.f, -1.f, 0.f));

		for (size_t i = 0; i < 8; i++)
		{
			if (adjTile[i]->m_tileDef->m_isSolid ||
				(adjTile[i]->m_tileDef->m_isWater && !actor->m_owner.IsValid() && actor->m_actorDef->m_faction != "Fish"))
			{
				if (PushZCylinderOutOfAABB3D(actorCenterXY, actorMinMaxZ, actorRadius, adjTile[i]->GetBound()))
				{
					actor->OnCollide(nullptr);
				}
			}
			else if (actor->m_actorDef->m_faction == "Fish" && (!adjTile[i]->m_tileDef->m_isWater && !adjTile[i]->m_tileDef->m_isBridge))
			{
				if (PushZCylinderOutOfAABB3D(actorCenterXY, actorMinMaxZ, actorRadius, adjTile[i]->GetBound()))
				{
					actor->OnCollide(nullptr);
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < m_tiles.size(); i++)
		{
			if (m_tiles[i]->m_tileDef->m_isSolid || (m_tiles[i]->m_tileDef->m_isWater && !actor->m_owner.IsValid()))
			{
				if (PushZCylinderOutOfAABB3D(actorCenterXY, actorMinMaxZ, actorRadius, m_tiles[i]->GetBound()))
				{
					actor->OnCollide(nullptr);
				}
			}
		}
	}

	if (actorMinMaxZ.m_max > MAP_HEIGHT)
	{
		float displacement = MAP_HEIGHT - actorMinMaxZ.m_max;
		actorMinMaxZ.m_max += displacement;
		actorMinMaxZ.m_min += displacement;
		actor->OnCollide(nullptr);
	}
	if (actorMinMaxZ.m_min < 0.f)
	{
		float displacement = 0.f - actorMinMaxZ.m_min;
		actorMinMaxZ.m_max += displacement;
		actorMinMaxZ.m_min += displacement;
		actor->OnCollide(nullptr);
	}
	actor->SetActorPosition(actorCenterXY, actorMinMaxZ);
}

void Map::DeleteAllGarbageActors()
{
	for (size_t i = 0; i < m_allActorsList.size(); i++)
	{
		if (m_allActorsList[i] == nullptr)
		{
			continue;
		}
		if (m_allActorsList[i]->m_isGarbage)
		{
			m_allActorsList[i]->m_UID = ActorUID::INVALID;
			delete m_allActorsList[i];
			m_allActorsList[i] = nullptr;
		}
	}
}

void Map::PopulateFishDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost)
{
	TileHeatMap& distField = out_distanceField;
	distField.SetHeaEverywhere(maxCost);
	distField.SetHeatAt(startCoords, 0.f);

	float currentValue = 0.f;
	bool isDirty = true;
	while (isDirty)
	{
		isDirty = false;
		float nextValue = currentValue + 1.f;
		for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
			{
				IntVec2 tileCoord = IntVec2(tileX, tileY);
				if (distField.GetHeatAt(tileCoord) == currentValue)
				{
					SetHeatForFishBehavior(isDirty, distField, tileCoord + IntVec2::EAST, nextValue);
					SetHeatForFishBehavior(isDirty, distField, tileCoord + IntVec2::WEST, nextValue);
					SetHeatForFishBehavior(isDirty, distField, tileCoord + IntVec2::NORTH, nextValue);
					SetHeatForFishBehavior(isDirty, distField, tileCoord + IntVec2::SOUTH, nextValue);
				}
			}
		}
		currentValue += 1.f;
	}

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			IntVec2 tileCoord = IntVec2(tileX, tileY);
			if (distField.GetHeatAt(tileCoord) == maxCost)
			{
				distField.SetHeatAt(tileCoord, -1.f);
			}
		}
	}
}

void Map::SetHeatForFishBehavior(bool& isDirty, TileHeatMap& distField, IntVec2 coords, float value)
{
	if (coords.x < 0 || coords.x >= distField.m_dimensions.x || coords.y < 0 || coords.y >= distField.m_dimensions.y)
	{
		return;
	}
	if (IsTileSolid(coords) || (!IsTileWater(coords) && !IsTileBridge(coords)))
	{
		return;
	}
	if (distField.GetHeatAt(coords) < value)
	{
		return;
	}

	distField.SetHeatAt(coords, value);
	isDirty = true;
}

int Map::AddActorToList(Actor* actor, ActorList& list)
{
	for (int i = 0; i < (int)list.size(); i++)
	{
		if (list[i] == nullptr)
		{
			list[i] = actor;
			return i;
		}
	}
	list.push_back(actor);
	return (int)list.size() - 1;
}

void Map::SpawnFishIfTotalFishLessThan(int numberOfMaxFish)
{
	if (m_fishCount >= numberOfMaxFish)
	{
		return;
	}

	if (m_fishSpawnTimer->DecrementPeriodIfElapsed())
	{
		int randIndexName = g_theRNG->RollRandomIntInRange(0, 9);
		int randIndexPos = g_theRNG->RollRandomIntInRange(0, (int)m_fishSpawnPosList.size() - 1);
		float yaw = g_theRNG->RollRandomFloatInRange(0.f, 360.f);

		Vec3 position = Vec3(m_fishSpawnPosList[randIndexPos].x + 0.5f, m_fishSpawnPosList[randIndexPos].y + 0.5f, 0.f);
		EulerAngles orientation = EulerAngles(yaw, 0.f, 0.f);
		ActorUID actorUID = SpawnActor(Stringf("Fish_%i", randIndexName), position, orientation);
		FishAIController* newFish = new FishAIController();
		newFish->Possess(actorUID);
		m_fishCount++;
		m_fishSpawnTimer->Start();
	}

}

void Map::AddSkyBoxVerts(std::vector<Vertex_PCU>& verts, float sizeRateVsMapDim /*= 1.f*/) const
{
	float radius = (m_dimensions.x >= m_dimensions.y) ? (float)m_dimensions.x * sizeRateVsMapDim : (float)m_dimensions.y * sizeRateVsMapDim;
	float rateBottom = 0.45f;
	//AddVertsForSphere(verts, Vec3(0, 0, 0), radius);
	AddVertsForSkyBox(verts, AABB3(-2, -2, -radius * rateBottom, radius + 2, radius + 2, radius * (1.f - rateBottom)), Rgba8::COLOR_WHITE);
}

void Map::RenderSkybox() const
{
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthStencilMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(m_mapDef->m_skyboxTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray((int)m_skyboxVertexes.size(), m_skyboxVertexes.data());
}