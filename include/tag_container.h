#pragma once

#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include <unordered_map>
#include <vector>

using namespace godot;

struct IdentityHash {
    size_t operator()(uint32_t k) const {
        return k;
    }
};

class TagContainer : public Resource {
    GDCLASS(TagContainer, Resource)

private:
    std::unordered_map<uint32_t, int32_t, IdentityHash> _tag_to_count_map;
    std::unordered_map<uint32_t, std::vector<Callable>, IdentityHash> _tag_to_callable_map;

protected:
    static void _bind_methods();

public:
    TagContainer() {}

    // Manipulate operations
    bool add_tag(const StringName& p_tag_name, int32_t p_count = 1);
    bool remove_tag(const StringName& p_tag_name, int32_t p_count = 1);
    void clear();
    
    // Query operations
    bool has_tag(const StringName& p_tag_name, bool p_is_exact = false) const;
    bool has_all(const Ref<TagContainer>& p_other, bool p_is_exact = false) const;
    bool has_any(const Ref<TagContainer>& p_other, bool p_is_exact = false) const;

    int32_t get_tag_count(const StringName& p_tag_name) const;
    int32_t get_num_tags() const { return _tag_to_count_map.size(); }

    // Event operations
    void watch_tag(const StringName& p_tag_name, const Callable& callback);
    void unwatch_tag(const StringName& p_tag_name, const Callable& callback);

    // Serialize/Deserialize
    PackedByteArray serialize() const;
    static Ref<TagContainer> deserialize(const PackedByteArray& bytes);

    // Debug
    PackedStringArray get_tag_names() const;

private:
    void _get_property_list(List<PropertyInfo> *p_list) const;
    bool _get(const StringName &p_name, Variant &r_ret) const;
    bool _set(const StringName &p_name, const Variant &p_value);

    void _notify_watchers(const StringName& p_tag_name, int32_t p_count);
    void _notify_watcher(const StringName& p_tag_notified, const StringName& p_tag_changed, int32_t p_count);
};
