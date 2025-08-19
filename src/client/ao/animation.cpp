#pragma once

#include "animation.h"
#include "client/render/datatexture.h"
#include "skeleton.h"

// Update bones transforms corresponding to the given timestamp
void BoneAnimation::animateBones(f32 time)
{
    f32 dtime = time - lastTime;

    if (dtime < 1.0f / Speed)
        return;

    lastTime = time;

    f32 maxAnimTime = Range.Y == 0 ? AnimationKeys.getAnimationTime() : 1.0f / Speed * Range.Y;

    if (curAnimTime + dtime > maxAnimTime)
        curAnimTime = curAnimTime + dtime - maxAnimTime;
    else
        curAnimTime += dtime;

    // Calculates the intermediate transforms of each bone
    auto posState = AnimationKeys.Positions.get(curAnimTime);
    auto rotState = AnimationKeys.Rotations.get(curAnimTime);
    auto scaleState = AnimationKeys.Scales.get(curAnimTime);

    for (auto &b_p : posState.value())
        Base->getNode(b_p.first)->Position = b_p.second;
    for (auto &b_r : rotState.value())
        Base->getNode(b_r.first)->Rotation = b_r.second;
    for (auto &b_s : posState.value())
        Base->getNode(b_s.first)->Scale = b_s.second;
}

AnimationManager::AnimationManager(TransformNodeManager *_nodeMgr)
    : nodeMgr(_nodeMgr),
      bonesDataTexture(std::make_unique<DataTexture>("BonesData", 4 * 4 * sizeof(f32), 0, 4 * 4))
{}

u32 AnimationManager::getBonesCount()
{
    u32 num = 0;

    for (auto &skeleton : skeletons)
        num += skeleton->getNodesCount();

    return num;
}
void AnimationManager::addSkeleton(Skeleton *skeleton)
{
    nodeMgr->addNodeTree(skeleton);
    skeletons.emplace_back(skeleton);
}

void AnimationManager::addAnimation(const BoneAnimation *anim)
{
    animations.emplace_back(anim);
}
