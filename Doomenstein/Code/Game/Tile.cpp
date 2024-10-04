#include "Tile.hpp"
#include "Game/GameCommon.hpp"

std::vector<TileDefinition*> TileDefinition::s_tileDefs;

Tile::Tile(IntVec2 tileCoords)
	:m_tileCoords(tileCoords)
{
}

Tile::~Tile()
{
}

AABB3 Tile::GetBound() const
{
	return m_bounds;
}

TileDefinition::TileDefinition(XmlElement& element)
	:m_name(ParseXmlAttribute(element, "name", "Invalid")),
	m_isSolid(ParseXmlAttribute(element, "isSolid", false)),
	m_hasSideFaces(ParseXmlAttribute(element, "hasSideFaces", false)),
	m_isWater(ParseXmlAttribute(element, "isWater", false)),
	m_isBridge(ParseXmlAttribute(element, "isBridge", false)),
	m_hasNorthFace(ParseXmlAttribute(element, "hasNorthFace", true)),
	m_hasSouthFace(ParseXmlAttribute(element, "hasSouthFace", true)),
	m_hasWestFace(ParseXmlAttribute(element, "hasWestFace", true)),
	m_hasEastFace(ParseXmlAttribute(element, "hasEastFace", true)),
	m_isRenderLit(ParseXmlAttribute(element, "isRenderLit", true)),
	m_bridgeNorthEnd(ParseXmlAttribute(element, "bridgeNorthEnd", false)),
	m_bridgeSouthEnd(ParseXmlAttribute(element, "bridgeSouthEnd", false)),
	m_bridgeWestEnd(ParseXmlAttribute(element, "bridgeWestEnd", false)),
	m_bridgeEastEnd(ParseXmlAttribute(element, "bridgeEastEnd", false)),
	m_isRenderOffset(ParseXmlAttribute(element, "isRenderOffset", false)),
	m_renderOffset(ParseXmlAttribute(element, "renderOffset", 0.f)),
	m_mapImagePixelColor(ParseXmlAttribute(element, "mapImagePixelColor", Rgba8::COLOR_WHITE)),
	m_floorSpriteCoord(ParseXmlAttribute(element, "floorSpriteCoords", IntVec2::ZERO)),
	m_ceilingSpriteCoord(ParseXmlAttribute(element, "ceilingSpriteCoords", IntVec2::ZERO)),
	m_wallSpriteCoord(ParseXmlAttribute(element, "wallSpriteCoords", IntVec2::ZERO)),
	m_waterGroundSpriteCoords(ParseXmlAttribute(element, "waterGroundSpriteCoords", IntVec2::ZERO)),
	m_waterBridgeSpriteCoords(ParseXmlAttribute(element, "waterBridgeSpriteCoords", IntVec2::ZERO))

{

}

void TileDefinition::InitializeTileDefs(char const* filePath)
{
	XmlDocument file;
	XmlError result = file.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, "FILE IS NOT LOADED");

	XmlElement* rootElement = file.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Root Element is null");

	XmlElement* tileDefElement = rootElement->FirstChildElement();

	while (tileDefElement)
	{
		std::string name = tileDefElement->Name();
		GUARANTEE_OR_DIE(name == "TileDefinition", "Root child element is in the wrong format");
		TileDefinition* newTileDef = new TileDefinition(*tileDefElement);
		s_tileDefs.push_back(newTileDef);
		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

void TileDefinition::ClearDefinition()
{
	for (size_t i = 0; i < s_tileDefs.size(); i++)
	{
		if (s_tileDefs[i] != nullptr)
		{
			delete s_tileDefs[i];
			s_tileDefs[i] = nullptr;
		}
	}
}