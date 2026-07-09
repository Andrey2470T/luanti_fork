#include "floodfill.h"
#include "nodedef.h"

void ClientMapLightFloodFill::addMapBlock(v3s16 blockpos, MapBlock *block)
{
    block->refGrab();
    m_mapblocks_light.emplace(blockpos, MapBlockLightInfo{block});
    addLightNodesInQueue(block, true);
}
void ClientMapLightFloodFill::removeMapBlock(v3s16 blockpos)
{
    auto it = m_mapblocks_light.find(blockpos);
    if (it != m_mapblocks_light.end()) {
        addLightNodesInQueue(it->second.block, false);
        /*if (it->second.block)
            it->second.block->refDrop();
        m_mapblocks_light.erase(it);*/
    }
}

void ClientMapLightFloodFill::addLightNodesInQueue(MapBlock *block, bool propagate)
{
    for (s16 z = 0; z < MAP_BLOCKSIZE; ++z) {
        for (s16 y = 0; y < MAP_BLOCKSIZE; ++y) {
            for (s16 x = 0; x < MAP_BLOCKSIZE; ++x) {
                v3s16 pos(x, y, z);
                auto node = block->getNodeNoCheck(pos);
                auto &cf = m_nodedef->get(node.getContent());

                if (cf.light_source > 0) {
                    if (propagate) {
                        m_light_propagation_queue.push({block->getPos(), pos});
                    } else {
                        m_light_removal_queue.push({block->getPos(), pos});
                    }
                }
            }
        }
    }
}

void ClientMapLightFloodFill::updateFill()
{
}
