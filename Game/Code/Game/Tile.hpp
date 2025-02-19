#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include <vector>
#include <string>

struct TileDefinition
{
	std::string m_name = "";
	bool m_isSolid = false;
	bool m_hasSideFaces = false;
	bool m_isWater = false;
	bool m_isBridge = false;

	bool m_bridgeNorthEnd = false;
	bool m_bridgeSouthEnd = false;
	bool m_bridgeWestEnd = false;
	bool m_bridgeEastEnd = false;

	bool m_hasNorthFace = true;
	bool m_hasSouthFace = true;
	bool m_hasWestFace	= true;
	bool m_hasEastFace	= true;

	bool m_isRenderLit = true;
	bool m_isRenderOffset = false;
	float m_renderOffset = 0.f;
	Rgba8 m_mapImagePixelColor = Rgba8::COLOR_WHITE;
	IntVec2 m_floorSpriteCoord;
	IntVec2 m_ceilingSpriteCoord;
	IntVec2 m_wallSpriteCoord;
	IntVec2 m_waterBridgeSpriteCoords;
	IntVec2 m_waterGroundSpriteCoords;
public:
	TileDefinition(XmlElement& element);
	static void InitializeTileDefs(char const* filePath);
	static void ClearDefinition();
	static std::vector<TileDefinition*> s_tileDefs;
};


class Tile
{
public:
	Tile(IntVec2 tileCoords);
	~Tile();

	AABB3 GetBound() const;

	const TileDefinition* m_tileDef;
	AABB3 m_bounds = AABB3::ZERO_TO_ONE;
	IntVec2	m_tileCoords = IntVec2(-1, -1);
};