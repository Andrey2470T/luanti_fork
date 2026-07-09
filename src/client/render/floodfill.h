#pragma once

#include "irr_v3d.h"
#include "mapblock.h"
#include "constants.h"
#include <queue>

class ClientMapLightFloodFill
{
    struct MapBlockLightInfo
    {
        MapBlock *block = nullptr;
        std::array<u16, MapBlock::nodecount> light={};
    };

    struct LightNodeEntry
    {
        v3s16 mapblockPos;
        v3s16 nodePos;
    };

    const NodeDefManager *m_nodedef;
    std::unordered_map<v3s16, MapBlockLightInfo> m_mapblocks_light;
    std::queue<LightNodeEntry> m_light_propagation_queue;
    std::queue<LightNodeEntry> m_light_removal_queue;

public:
    ClientMapLightFloodFill(const NodeDefManager *nodedef) : m_nodedef(nodedef) {}

    void addMapBlock(v3s16 blockpos, MapBlock *block);
    void removeMapBlock(v3s16 blockpos);

    void addLightNodesInQueue(MapBlock *block, bool propagate=true);

    void updateFill();
};