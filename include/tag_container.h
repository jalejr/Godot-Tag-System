#pragma once

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "identity_hash.h"
#include "tag_manager.h"
#include "tag_watcher.h"
#include <unordered_map>

using namespace godot;

class TagContainer : public Node {
    GDCLASS(TagContainer, Node)

private:
    TagManager* _tag_manager = nullptr;
    mutable std::unordered_map<uint32_t, int32_t, IdentityHash> _tag_to_count_map;
    std::unordered_map<uint32_t, uint32_t, IdentityHash> _ancestor_to_count;
    TagWatcher _watcher;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    TagContainer() = default;

    void _ready() override;

    // Manipulate operations
    bool add_tag(const StringName& p_tag_name, int32_t p_count = 1);
    void add_tags(const TypedArray<StringName>& p_tags);
    bool remove_tag(const StringName& p_tag_name, int32_t p_count = 1);
    void remove_tags(const TypedArray<StringName>& p_tags);
    void clear();
    
    // Query operations
    bool has_tag(const StringName& p_tag_name, bool p_is_exact = false) const;
    bool has_all_tags(const TypedArray<StringName>& p_tags, bool p_is_exact = false) const;
    bool has_any_tags(const TypedArray<StringName>& p_tags, bool p_is_exact = false) const;
    bool has_all_in_container(TagContainer* p_other, bool p_is_exact = false) const;
    bool has_any_in_container(TagContainer* p_other, bool p_is_exact = false) const;

    int32_t get_tag_count(const StringName& p_tag_name) const;
    int32_t get_num_tags() const { return _tag_to_count_map.size(); }

    // Event operations
    void watch_tag(const StringName& p_tag_name, const Callable& callback);
    void unwatch_tag(const StringName& p_tag_name, const Callable& callback);

    // Serialize/Deserialize
    PackedByteArray serialize() const;
    static TagContainer* deserialize(const PackedByteArray& bytes);

    // Debug
    PackedStringArray get_tag_names() const;

private:
    bool _resolve_index(const StringName& p_tag_name, uint32_t& r_index) const;
    bool _has_tag_by_index(uint32_t p_query_idx, bool p_is_exact) const;
};
