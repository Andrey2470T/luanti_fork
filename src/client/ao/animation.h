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

        Frame(f32 _time, const std::map<u8, T>& _values) : time(_time), values(_values) {}

        Frame() = default;

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
    }

    void append(const KeyChannel<T> &other) {
        Frames.insert(other.Frames.begin(), other.Frames.end());
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

        auto next = std::lower_bound(Frames.begin(), Frames.end(), time, [](const auto& frame, f32 time) {
            return frame.time < time;
        });
        if (next == Frames.begin())
            return next->values;
        if (next == Frames.end())
            return std::prev(next)->values;

        auto prev = std::prev(next);
        auto prev_values = std::prev(next)->values;
        auto next_values = next->values;

        switch (InterpMode) {
        case KeyChannelInterpMode::CONSTANT:
            return prev_values;
        case KeyChannelInterpMode::LINEAR: {
            std::map<u8, T> vals;

            for (u8 i = 0; i < std::min(prev_values.size(), next_values.size()); i++) {
                vals[i] = interpolateValue(
                    prev_values[i], next_values[i],
                    (time - prev->time) / (next->time - prev->time)
                );
            }

            return vals;
        }}

        return prev_values;
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

    f32 curAnimTime = 0.0f;

    bool animStarted = false;
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

    bool isStarted() const
    {
        return animStarted;
    }
    void start()
    {
        curAnimTime = 1.0f / Speed * Range.X;
        animStarted = true;
    }
    void stop()
    {
        animStarted = false;
    }
    // Update bones transforms corresponding to the given timestamp
    bool animateBones(f32 dtime);
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
    void addAnimation(BoneAnimation *anim);
};
