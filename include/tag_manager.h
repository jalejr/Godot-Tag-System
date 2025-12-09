#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace godot;

struct Tag {
    StringName name;
    uint32_t index;
};

class TagManager : public Node {
	GDCLASS(TagManager, Node)

private:
    std::vector<Tag> _tags;
    std::unordered_map<StringName, size_t> _name_to_index_map;
    std::unordered_map<uint32_t, uint32_t> _direct_parent_map; // temporary child to direct parent
    std::unordered_map<uint32_t, std::vector<uint32_t>> _parent_map; // child to ancestors
    std::unordered_set<uint64_t> _parent_lookup_table; // flattened table for O(1) access

    bool _is_locked = false;

    static TagManager* _singleton;

protected:
    static void _bind_methods();

public:
    static constexpr uint32_t INVALID_INDEX = UINT32_MAX;

    TagManager();
    ~TagManager();

    static TagManager* get_singleton() { return _singleton; }

    // Registration
    void register_tag(const String& p_tag_name);
    void register_tags_from_array(const PackedStringArray& p_tag_names);
    void lock_registry(); // Construct parent_lookup table and prevent altering after

    // Query
    uint32_t get_tag_index(const StringName& p_tag_name) const;
    StringName get_tag_name(uint32_t P_index) const;
    bool is_valid_tag(const StringName& p_tag_name) const;
    bool is_parent_of(uint32_t p_parent_index, uint32_t p_child_index) const;
    PackedStringArray get_parent_tags(const StringName& p_tag_name) const;

    // Debugging purposes
    int32_t get_tag_count() const { return _tags.size(); }
    PackedStringArray get_all_tag_names() const;
};
