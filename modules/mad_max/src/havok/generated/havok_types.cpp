#include "havok/generated/havok_types.h"

#include <bit>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "havok/legacy/sdk2014/packfile.hpp"

namespace HavokTypes {
    std::unordered_map<std::string_view, Havok2014::ClassKind> &havok_type_info() {
        static std::unordered_map<std::string_view, Havok2014::ClassKind> registered;
        return registered;
    }
    namespace {
        using json = nlohmann::json;

        hkStringPtr string_ptr(const Havok2014::Packfile &file, const Havok2014::Location at) {
            return {std::string(file.string(file.required_pointer(at)))};
        }

        hkaBone read_bone(const Havok2014::Packfile &file, Havok2014::Location at) {
            file.require(at, 16);
            return {string_ptr(file, at), file.u8(file.field(at, 8, 1)) != 0};
        }

        hkVector4f read_vector(const Havok2014::Packfile &file, Havok2014::Location at) {
            hkVector4f vector;
            for (size_t i = 0; i < 4; ++i)
                vector.value[i] = file.f32(file.field(at, i * 4, 4));
            return vector;
        }

        hkQsTransform read_pose(const Havok2014::Packfile &file, Havok2014::Location at) {
            file.require(at, 48);
            return {
                read_vector(file, at), {read_vector(file, file.field(at, 16, 16))},
                read_vector(file, file.field(at, 32, 16))
            };
        }

        hkaSkeleton read_skeleton(const Havok2014::Packfile &file, Havok2014::Location at) {
            file.expect_class(at, Havok2014::ClassKind::skeleton);
            file.require(at, 136);
            // Havok 2013's hkReferencedObject base is 16 bytes, with no property bag.
            if (file.pointer(file.field(at, 0, 8)))
                Havok2014::invalid("skeleton has unsupported base object pointer");
            hkaSkeleton result;
            result.memSizeAndFlags = file.u16(file.field(at, 8, 2));
            result.refCount = file.u16(file.field(at, 10, 2));
            result.name = string_ptr(file, file.field(at, 16, 8));
            const auto parents = file.array(file.field(at, 24, 16), 2);
            const auto bones = file.array(file.field(at, 40, 16), 16);
            const auto poses = file.array(file.field(at, 56, 16), 48);
            if (!parents.size || parents.size != bones.size || bones.size != poses.size)
                Havok2014::invalid("inconsistent skeleton bone arrays");
            result.parentIndices.reserve(bones.size);
            result.bones.reserve(bones.size);
            result.referencePose.reserve(bones.size);
            for (size_t i = 0; i < bones.size; ++i) {
                const auto parent = file.i16(file.element(parents, i, 2));
                if (parent < -1 || (parent >= 0 && size_t(parent) >= bones.size))
                    Havok2014::invalid("invalid parent bone index");
                result.parentIndices.push_back(parent);
                result.bones.push_back(read_bone(file, file.element(bones, i, 16)));
                result.referencePose.push_back(read_pose(file, file.element(poses, i, 48)));
            }
            const auto floats = file.array(file.field(at, 72, 16), 4);
            result.referenceFloats.reserve(floats.size);
            for (size_t i = 0; i < floats.size; ++i)
                result.referenceFloats.push_back(file.f32(file.element(floats, i, 4)));
            const auto slots = file.array(file.field(at, 88, 16), 8);
            if (slots.size != floats.size) Havok2014::invalid("reference float/slot count mismatch");
            result.floatSlots.reserve(slots.size);
            for (size_t i = 0; i < slots.size; ++i)
                result.floatSlots.push_back(string_ptr(file, file.element(slots, i, 8)));
            const auto frames = file.array(file.field(at, 104, 16), 16);
            if (frames.size) Havok2014::invalid("nonempty localFrames require unsupported hkLocalFrame decoding");
            const auto partitions = file.array(file.field(at, 120, 16), 16);
            result.partitions.reserve(partitions.size);
            for (size_t i = 0; i < partitions.size; ++i) {
                const auto item = file.element(partitions, i, 16);
                result.partitions.push_back({
                    string_ptr(file, item), file.i16(file.field(item, 8, 2)),
                    file.u16(file.field(item, 10, 2))
                });
            }
            return result;
        }

        struct hkaAnimationContainer {
            std::vector<Havok2014::Location> skeletons;

            static hkaAnimationContainer read(const Havok2014::Packfile &file, Havok2014::Location at) {
                file.expect_class(at, Havok2014::ClassKind::animationContainer);
                file.require(at, 96);
                const auto entries = file.array(file.field(at, 16, 16), 8);
                hkaAnimationContainer result;
                result.skeletons.reserve(entries.size);
                for (size_t i = 0; i < entries.size; ++i)
                    result.skeletons.push_back(file.required_pointer(file.element(entries, i, 8)));
                return result;
            }
        };

        struct hkRootLevelContainer {
            std::vector<Havok2014::Location> animationContainers;

            static hkRootLevelContainer read(const Havok2014::Packfile &file) {
                const auto variants = file.array(file.root(), 24);
                hkRootLevelContainer result;
                for (size_t i = 0; i < variants.size; ++i) {
                    const auto variant = file.element(variants, i, 24);
                    const auto class_name = file.pointer(file.field(variant, 8, 8));
                    if (!class_name || Havok2014::Packfile::class_kind(file.string(*class_name)) !=
                        Havok2014::ClassKind::animationContainer)
                        continue;
                    result.animationContainers.push_back(file.required_pointer(file.field(variant, 16, 8)));
                }
                return result;
            }
        };

        template<typename T>
        json values_to_json(const std::vector<T> &values) {
            json array = json::array();
            array.get_ref<json::array_t &>().reserve(values.size());
            for (const auto &value: values) array.push_back(value.to_json());
            return array;
        }
    }

    nlohmann::json hkStringPtr::to_json() const { return {{"stringAndFlag", stringAndFlag}}; }
    nlohmann::json hkVector4f::to_json() const { return value; }
    nlohmann::json hkQuaternionf::to_json() const { return {{"vec", vec.to_json()}}; }

    nlohmann::json hkQsTransform::to_json() const {
        return {
            {"translation", translation.to_json()}, {"rotation", rotation.to_json()},
            {"scale", scale.to_json()}
        };
    }

    nlohmann::json hkaBone::to_json() const {
        return {{"name", name.to_json()}, {"lockTranslation", lockTranslation}};
    }

    nlohmann::json hkaSkeleton_Partition::to_json() const {
        return {{"name", name.to_json()}, {"startBoneIndex", startBoneIndex}, {"numBones", numBones}};
    }

    nlohmann::json hkaSkeleton::to_json() const {
        return {
            {"memSizeAndFlags", memSizeAndFlags}, {"refCount", refCount}, {"name", name.to_json()},
            {"parentIndices", parentIndices}, {"bones", values_to_json(bones)},
            {"referencePose", values_to_json(referencePose)}, {"referenceFloats", referenceFloats},
            {"floatSlots", values_to_json(floatSlots)}, {"localFrames", json::array()},
            {"partitions", values_to_json(partitions)}
        };
    }

    void init_havok_type_info() {
        auto &registered = havok_type_info();
        registered.reserve(3);
        registered.emplace("hkRootLevelContainer", Havok2014::ClassKind::root);
        registered.emplace("hkaAnimationContainer", Havok2014::ClassKind::animationContainer);
        registered.emplace("hkaSkeleton", Havok2014::ClassKind::skeleton);
    }

    std::vector<hkaSkeleton> read_skeletons(std::span<const uint8_t> bytes) {
        const Havok2014::Packfile file(bytes);
        std::vector<hkaSkeleton> result;
        std::unordered_set<Havok2014::Location, Havok2014::LocationHash> seen_skeletons;
        for (const auto container: hkRootLevelContainer::read(file).animationContainers)
            for (const auto skeleton: hkaAnimationContainer::read(file, container).skeletons)
                if (seen_skeletons.insert(skeleton).second)
                    result.push_back(read_skeleton(file, skeleton));
        if (result.empty()) Havok2014::invalid("no linked hkaSkeleton objects");
        return result;
    }
}
