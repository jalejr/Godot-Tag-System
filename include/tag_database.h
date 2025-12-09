#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

class TagDatabase : public Resource {
    GDCLASS(TagDatabase, Resource)

private:
    PackedStringArray _tag_names;
    Dictionary _tag_metadata;

protected:
    static void _bind_methods();

public:
    TagDatabase() {}

    // Manipulate operations
    void add_tag(const String& p_tag_name);
    void remove_tag(const String& p_tag_name);
    void set_all_tags(const PackedStringArray& p_tags);

    // Query operations
    bool has_tag(const String& p_tag_name) const;
    PackedStringArray get_all_tags() const;
    PackedStringArray get_root_tags() const;
    PackedStringArray get_children(const String& p_parent_tag_name) const;

    // Metadata operations
    void set_tag_description(const String& p_tag_name, const String& p_description);
    String get_tag_description(const String& p_tag_name) const;

    void register_with_manager() const;

    // Generation operations
    String generate_gdscript_constants() const;
    String generate_cpp_constants() const;
};
