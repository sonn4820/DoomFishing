#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Actor.hpp"
#include "Game/Tile.hpp"
#include "Engine/Core/HeatMaps.hpp"


class Game;
class PlayerController;

constexpr float MAP_HEIGHT = 10.f;

struct MapDefinition
{
public:
	std::string m_name = "";
	Image m_image;
	Shader* m_shader = nullptr;
	Texture* m_spriteSheetTexture = nullptr;
	Texture* m_skyboxTexture = nullptr;
	IntVec2 m_spriteSheetCellCount;

	std::vector<ActorDefinition*> m_actorDefs;
	std::vector <Vec3> m_actorPosition;
	std::vector <EulerAngles> m_actorOrientation;
	std::vector <Vec3> m_actorVelocity;
public:
	MapDefinition(XmlElement& element);
	void SetSpawnInfo(XmlElement& element);
	static void InitializeMapDefs(char const* filePath);
	static void ClearDefinition();
	static MapDefinition* GetByName(std::string const& name);
	static std::vector<MapDefinition*> s_mapDefs;
};

struct RaycastResult
{
	// Basic raycast result information (required)
	bool	m_didImpact = false;
	float	m_impactDist = 0.f;
	Vec3	m_impactPos;
	Vec3	m_impactNormal;
	IntVec2 m_impactTileCoord;
	ActorUID m_targetUID = ActorUID::INVALID;

	// Original raycast information (optional)
	Vec3	m_rayFwdNormal;
	Vec3	m_rayStartPos;
	float	m_rayMaxLength = 1.f;
};

class Map
{
public:
	Map(MapDefinition* mapDef);
	~Map() = default;

	void StartUp();
	void Update();
	void Render(int playerID) const;
	void Shutdown();

	void HandleInput();

	// TILES
	void PopulateTileMap();
	void AddVertsForTile(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, int tileCoord) const;
	void SetTileType(int index, Rgba8 pixelColor);
	void SetTileType(int x, int y, Rgba8 pixelColor);
	int GetTileIndexesFromCoord(int xCoord, int yCoord) const;
	int GetTileIndexesFromCoord(IntVec2 coord) const;
	int GetTileXFromIndexes(int index) const;
	int GetTileYFromIndexes(int index) const;
	void RenderTile() const;
	bool IsTileSolid(IntVec2 coord) const;
	bool IsTileWater(IntVec2 coord) const;
	bool IsTileBridge(IntVec2 coord) const;
	bool IsTileInBound(IntVec2 coord) const;
	bool IsTileInBoundFromPos(Vec3 pos) const;
	bool IsPositionInBound(float x, float y, float tolerance);
	Tile* GetTile(IntVec2 coord) const;
	Tile* GetTileFromPos(Vec3 pos) const;

	// SKYBOX

	void RenderSkybox() const;
	void AddSkyBoxVerts(std::vector<Vertex_PCU>& verts, float sizeRateVsMapDim = 1.f) const;

	//RAYCAST
	RaycastResult RaycastWorldXY(Vec3 startPos, Vec3 fwdNormal, float maxDist) const;
	RaycastResult RaycastWorldZ(Vec3 startPos, Vec3 fwdNormal, float maxDist) const;
	RaycastResult RaycastActors(Vec3 startPos, Vec3 fwdNormal, float maxDist, ActorUID ownerActor) const;
	RaycastResult RaycastAll(Vec3 startPos, Vec3 fwdNormal, float maxDist, ActorUID ownerActor) const;

	// ACTOR
	ActorUID SpawnActor(std::string actorDefName, Vec3 position, EulerAngles orientation = EulerAngles(), Rgba8 solidColor = Rgba8::COLOR_WHITE, Rgba8 wireframeColor = Rgba8::COLOR_WHITE, ActorUID owner = ActorUID::INVALID);
	void SpawnPlayer(int playerID);
	void PopulateActors();
	void GenerateUID();
	Actor* GetActorByUID(ActorUID actorUID);
	ActorUID GetClosestVisibleEnemy(ActorUID searcher, std::string enemyFaction);
	void DebugPossessNext();
	void AddAllActorsCanBePossess();
	void CollideActors();
	void CollideActorVsActor(Actor* actorA, Actor* actorB);
	void CollideActorsVsMap();
	void CollideActorVsMap(Actor* actor);
	void DeleteAllGarbageActors();

	//HEATMAP
	void PopulateFishDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost);
	void SetHeatForFishBehavior(bool& isDirty, TileHeatMap& distField, IntVec2 coords, float value);
public:

	// MAP INFO
	MapDefinition* m_mapDef = nullptr;
	IntVec2 m_dimensions;
	static const unsigned s_maxSalt = 0x0000fffeu;
	unsigned int m_actorSalt = 0x0000fffeu;

	ActorList m_allActorsList;
	std::vector<ActorUID> m_actorsCanBePossessedList;
	int m_debugPossessedIndex = 0;
	ActorUID m_playerUID[2];

	// MAP DRAWING
	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	std::vector<Vertex_PCU> m_skyboxVertexes;
	std::vector<Tile*> m_tiles;
	Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

	float m_sunX = -10.f;
	float m_sunY = -10.f;
	float m_sunIntensity = 0.85f;
	float m_ambIntensity = 0.45f;

	std::vector<IntVec2> m_fishSpawnPosList;
	Timer* m_fishSpawnTimer = nullptr;
	int m_fishCount = 0;
private:
	int AddActorToList(Actor* actor, ActorList& list);
	void SpawnFishIfTotalFishLessThan(int numberOfMaxFish);
};

