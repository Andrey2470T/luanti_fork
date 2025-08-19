#pragma once

#include <BasicIncludes.h>
#include <set>
#include <Utils/Quaternion.h>
#include <optional>
#include <map>
#include <memory>

enum class KeyChannelInterpMode
{
    CONSTANT,
    LINEAR
};

// Key channels are independent frames from each other saving either
// positions or rotations or scales keys
template <class T>
struct KeyChannel {
    struct Frame {
        f32 time;
        std::map<u8, T> values;

        bool operator<(const Frame &other) const
        {
            return time < other.time;
        }
    };
    // The frames must be sorted by their timestamps and not be repeated
    std::set<Frame> Frames;
    KeyChannelInterpMode InterpMode;

    bool empty() const {
        return Frames.empty();
    }

    f32 getAnimationTime() const {
        return Frames.empty() ? 0.0f : std::prev(Frames.end())->time;
    }

    void append(f32 time, const std::map<u8, T> &values) {
        Frames.emplace(time, values);
        sortFrameBonesValues(time);
    }

    void append(const KeyChannel<T> &other) {
        Frames.insert(other.Frames.begin(), other.Frames.end());

        for (auto &f : other.Frames)
            sortFrameBonesValues(f.time);
    }

    void sortFrameBonesValues(f32 time)
    {
        auto frame = std::find_if(Frames.begin(), Frames.end(), [&time] (const Frame &f)
        {
            return f.time == time;
        });

        if (frame == Frames.end())
            return;

        std::sort(frame->values.begin(), frame->values.end(), [] (const std::pair<u8, T> &b1, const std::pair<u8, T> &b2)
        {
            return b1.first < b2.first;
        });
    }

    static Quaternion interpolateValue(Quaternion from, Quaternion to, f32 time) {
        Quaternion result;
        result.slerp(from, to, time);
        return result;
    }

    static v3f interpolateValue(v3f from, v3f to, f32 time) {
        // Note: `from` and `to` are swapped here compared to quaternion slerp
        return to.linInterp(from, time);
    }

    std::optional<std::map<u8, T>> get(f32 time) const {
        if (Frames.empty())
            return std::nullopt;

        const auto next = std::lower_bound(Frames.begin(), Frames.end(), time, [](const auto& frame, f32 time) {
            return frame.time < time;
        });
        if (next == Frames.begin())
            return next->values;
        if (next == Frames.end())
            return std::prev(next)->values;

        const auto prev = std::prev(next);

        switch (InterpMode) {
        case KeyChannelInterpMode::CONSTANT:
            return prev->values;
        case KeyChannelInterpMode::LINEAR: {
            std::map<u8, T> vals;

            for (u8 i = 0; i < std::min(prev->values.size(), next->values.size()); i++) {
                vals.emplace_back(i, interpolateValue(
                    prev->values[i].second, next->values[i].second,
                    (time - prev->time) / (next->time - prev->time)
                ));
            }

            return vals;
        }}
    }
};

struct Keys {
    KeyChannel<v3f> Positions;
    KeyChannel<Quaternion> Rotations;
    KeyChannel<v3f> Scales;

    bool empty() const {
        return Positions.empty() && Rotations.empty() && Scales.empty();
    }

    f32 getAnimationTime() const {
        return std::max({
            Positions.getAnimationTime(),
            Rotations.getAnimationTime(),
            Scales.getAnimationTime()
        });
    }
};

class Skeleton;

class BoneAnimation {
    Keys AnimationKeys;

    Skeleton *Base = nullptr;

    // 'X' is the start frame, 'Y' is the end one
    v2i Range;
    // frames per second
    f32 Speed = 15.0f;
    bool Looped = false;

    f32 lastTime = 0.0f;

    f32 curAnimTime = 0.0f;
public:
    BoneAnimation() = default;

    Skeleton *getBase() const
    {
        return Base;
    }
    void setBase(Skeleton *base)
    {
        Base = base;
    }

    u32 getMaxFrameCount() const
    {
        return std::max({
            AnimationKeys.Positions.Frames.size(),
            AnimationKeys.Rotations.Frames.size(),
            AnimationKeys.Scales.Frames.size()
        });
    }

    // Setters/getters below will be needed mostly for obj:set/get_animation()
    v2i getRange() const
    {
        return Range;
    }
    void setRange(const v2i &r)
    {
        Range = r;
    }

    f32 getFPS() const
    {
        return Speed;
    }
    void setFPS(f32 speed)
    {
        Speed = speed;
    }

    bool getLooped() const
    {
        return Looped;
    }
    void setLooped(bool loop)
    {
        Looped = loop;
    }

    void appendKeys(const Keys &other) {
        AnimationKeys.Positions.append(other.Positions);
        AnimationKeys.Rotations.append(other.Rotations);
        AnimationKeys.Scales.append(other.Scales);
    }

    void appendPositionKey(f32 time, const std::map<u8, v3f> &values)
    {
        AnimationKeys.Positions.append(time, values);
    }
    void appendRotationKey(f32 time, const std::map<u8, Quaternion> &values)
    {
        AnimationKeys.Rotations.append(time, values);
    }
    void appendScaleKey(f32 time, const std::map<u8, v3f> &values)
    {
        AnimationKeys.Scales.append(time, values);
    }

    void setInterpModes(KeyChannelInterpMode posInterp, KeyChannelInterpMode rotInterp, KeyChannelInterpMode scaleInterp)
    {
        AnimationKeys.Positions.InterpMode = posInterp;
        AnimationKeys.Rotations.InterpMode = rotInterp;
        AnimationKeys.Scales.InterpMode = scaleInterp;
    }
    // 'time' is the global time
    void setStartTime(f32 time)
    {
        lastTime = time;
        curAnimTime = 1.0f / Speed * Range.X;
    }
    // Update bones transforms corresponding to the given timestamp
    void animateBones(f32 time);
};

class DataTexture;
class TransformNodeManager;

class AnimationManager
{
    TransformNodeManager *nodeMgr;

    // Skeletons (aka TransformNodeTree) are saved in TransformNodeManager
    std::vector<Skeleton *> skeletons;
    std::vector<std::unique_ptr<BoneAnimation>> animations;

    std::unique_ptr<DataTexture> bonesDataTexture;
public:
    AnimationManager(TransformNodeManager *_nodeMgr);

    DataTexture *getBonesTexture() const
    {
        return bonesDataTexture.get();
    }
    u32 getSkeletonCount()
    {
        return skeletons.size();
    }
    u32 getBonesCount();
    void addSkeleton(Skeleton *skeleton);
    void addAnimation(const BoneAnimation *anim);
};
