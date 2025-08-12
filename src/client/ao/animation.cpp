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
        Base->getBone(b_p.first)->Transform.Position = b_p.second;
    for (auto &b_r : rotState.value())
        Base->getBone(b_r.first)->Transform.Rotation = b_r.second;
    for (auto &b_s : posState.value())
        Base->getBone(b_s.first)->Transform.Scale = b_s.second;
}

AnimationManager::AnimationManager()
    : bonesDataTexture(std::make_unique<DataTexture>("BonesData", 4 * 4 * sizeof(f32), 0, 4 * 4))
{}

u32 AnimationManager::getBonesCount()
{
    u32 num = 0;

    for (auto &skeleton : skeletons)
        num += skeleton->getUsedBonesCount();
}
void AnimationManager::addSkeleton(Skeleton *skeleton)
{
    skeletons.emplace_back(std::unique_ptr<Skeleton>(skeleton));
}

void AnimationManager::addAnimations(u32 skeletonN, const std::vector<BoneAnimation *> anims)
{
    for (auto &anim : anims)
        animations.emplace_back(skeletonN, std::unique_ptr<BoneAnimation>(anim));
}
