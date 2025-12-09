#include "tag_database.h"
#include "tag_manager.h"


void TagDatabase::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("add_tag", "tag_name"), &TagDatabase::add_tag);
    ClassDB::bind_method(D_METHOD("remove_tag", "tag_name"), &TagDatabase::remove_tag);
    ClassDB::bind_method(D_METHOD("set_all_tags", "tag_names"), &TagDatabase::set_all_tags);

    ClassDB::bind_method(D_METHOD("has_tag", "tag_name"), &TagDatabase::has_tag);
    ClassDB::bind_method(D_METHOD("get_all_tags"), &TagDatabase::get_all_tags);
    ClassDB::bind_method(D_METHOD("get_root_tags"), &TagDatabase::get_root_tags);
    ClassDB::bind_method(D_METHOD("get_children", "parent_name"), &TagDatabase::get_children);
    
    ClassDB::bind_method(D_METHOD("set_tag_description", "tag_name", "description"), 
                        &TagDatabase::set_tag_description);
    ClassDB::bind_method(D_METHOD("get_tag_description", "tag_name"), 
                        &TagDatabase::get_tag_description);
    
    ClassDB::bind_method(D_METHOD("register_with_manager"), &TagDatabase::register_with_manager);
    ClassDB::bind_method(D_METHOD("generate_gdscript_constants"), &TagDatabase::generate_gdscript_constants);
    ClassDB::bind_method(D_METHOD("generate_cpp_header"), &TagDatabase::generate_cpp_constants);
    
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tag_paths"), 
                 "set_all_tags", "get_all_tags");
}

void TagDatabase::add_tag(const String& p_tag_name)
{
    if (p_tag_name.is_empty()) {
        return;
    }
    
    PackedStringArray segments = p_tag_name.split(".");
    String path;
    
    for (int i = 0; i < segments.size(); i++) {
        if (i == 0) {
            path = segments[i];
        } else {
            path = path + "." + segments[i];
        }
        
        // Add this level if not already present
        if (!_tag_names.has(path)) {
            _tag_names.append(path);
        }
    }
}

void TagDatabase::remove_tag(const String& p_tag_name)
{
    int idx = _tag_names.find(p_tag_name);
    if (idx >= 0) {
        _tag_names.remove_at(idx);
    }

    if (_tag_metadata.has(p_tag_name)) {
        _tag_metadata.erase(p_tag_name);
    }
}

void TagDatabase::set_all_tags(const PackedStringArray& p_tags)
{
    _tag_names = p_tags;
}

bool TagDatabase::has_tag(const String& p_tag_name) const
{
    return _tag_names.has(p_tag_name);
}

PackedStringArray TagDatabase::get_all_tags() const
{
    return _tag_names;
}

PackedStringArray TagDatabase::get_root_tags() const
{
    PackedStringArray roots;
    
    for (int i = 0; i < _tag_names.size(); i++) {
        String path = _tag_names[i];
        if (!path.contains(".")) {
            roots.append(path);
        }
    }
    
    return roots;
}

PackedStringArray TagDatabase::get_children(const String& p_parent_tag_name) const
{
    PackedStringArray children;
    String prefix = p_parent_tag_name + String(".");

    for (int i = 0; i < _tag_names.size(); i++) {
        String path = _tag_names[i];
        
        if (path.begins_with(prefix)) {
            // Extract immediate child only
            String remainder = path.substr(prefix.length());
            if (!remainder.contains(".")) {
                children.append(path);
            }
        }
    }
    
    return children;
}

void TagDatabase::set_tag_description(const String& p_tag_name, const String& p_description)
{
    _tag_metadata[p_tag_name] = p_description;
}

String TagDatabase::get_tag_description(const String& p_tag_name) const
{
    if (_tag_metadata.has(p_tag_name)) {
        return _tag_metadata[p_tag_name];
    }
    return "";
}

void TagDatabase::register_with_manager() const
{
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) {
        ERR_PRINT("TagManager not initialized");
        return;
    }
    
    mgr->register_tags_from_array(_tag_names);
    mgr->lock_registry();
}

String TagDatabase::generate_gdscript_constants() const
{
    String output = "# AUTO-GENERATED - DO NOT EDIT\n";
    output += "# Generated from TagDatabase\n";
    output += "# Regenerate by clicking 'Generate Constants' in Tags editor\n\n";
    output += "class_name Tag\n\n";
    
    for (int i = 0; i < _tag_names.size(); i++) {
        String path = _tag_names[i];
        String const_name = path.replace(".", "_").to_upper();
        output += "const " + const_name + " = \"" + path + "\"\n";
    }
    
    return output;
}

String TagDatabase::generate_cpp_constants() const
{
    String output = "// AUTO-GENERATED - DO NOT EDIT\n";
    output += "// Generated from TagDatabase\n";
    output += "#pragma once\n\n";
    output += "namespace Tag {\n\n";
    
    for (int i = 0; i < _tag_names.size(); i++) {
        String path = _tag_names[i];
        String const_name = path.replace(".", "_").to_upper();
        output += "    inline constexpr const char* " + const_name + " = \"" + path + "\";\n";
    }
    
    output += "\n} // namespace Tags\n";
    
    return output;
}
