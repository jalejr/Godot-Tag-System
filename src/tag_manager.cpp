#include "tag_manager.h"
#include <vector>

TagManager* TagManager::_singleton = nullptr;

TagManager::TagManager()
{
    ERR_FAIL_COND(_singleton != nullptr);
    _singleton = this;
}

TagManager::~TagManager() {
    if (_singleton == this) {
        _singleton = nullptr;
    }
}

void TagManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("register_tag", "tag_name"), &TagManager::register_tag);
    ClassDB::bind_method(D_METHOD("register_tags_from_array", "tag_names"), &TagManager::register_tags_from_array);
    ClassDB::bind_method(D_METHOD("lock_registry"), &TagManager::lock_registry);
    
    ClassDB::bind_method(D_METHOD("get_tag_index", "tag_name"), &TagManager::get_tag_index);
    ClassDB::bind_method(D_METHOD("get_tag_name", "index"), &TagManager::get_tag_name);
    ClassDB::bind_method(D_METHOD("is_valid_tag", "tag_name"), &TagManager::is_valid_tag);
    
    ClassDB::bind_method(D_METHOD("get_tag_count"), &TagManager::get_tag_count);
    ClassDB::bind_method(D_METHOD("get_all_tag_names"), &TagManager::get_all_tag_names);
}

void TagManager::register_tag(const String& p_tag_name)
{
    if (_is_locked) {
            ERR_PRINT("Cannot register tags after registry is locked");
            return;
        }
    
    if (p_tag_name.is_empty()) {
        return;
    }
    
    PackedStringArray segments = p_tag_name.split(".");
    uint32_t parent_idx = INVALID_INDEX;
    String current_path;
    
    for (int i = 0; i < segments.size(); i++) {
        String segment = segments[i];
        
        // Build full path
        if (i == 0) {
            current_path = segment;
        } else {
            current_path = current_path + "." + segment;
        }
        
        StringName full_name(current_path);
        
        // Check if already registered
        auto it = _name_to_index_map.find(full_name);
        if (it != _name_to_index_map.end()) {
            parent_idx = _tags[it->second].index;
            continue;
        }
        
        // Create new tag
        Tag tag;
        tag.name = full_name;
        tag.index = _tags.size();
        
        _tags.push_back(tag);
        _name_to_index_map[full_name] = _tags.size() - 1;
        
        // Track direct parent relationship
        if (parent_idx != INVALID_INDEX) {
            _direct_parent_map[tag.index] = parent_idx;
        }
        
        parent_idx = tag.index;
    }
}

void TagManager::register_tags_from_array(const PackedStringArray& p_tag_names)
{
    for (int i = 0; i < p_tag_names.size(); i++) {
        register_tag(p_tag_names[i]);
    }
}

void TagManager::lock_registry()
{
    if (_is_locked) {
        return;
    }
    
    _is_locked = true;
    
    // Build full parent_map by walking up direct parent chain
    for (size_t i = 0; i < _tags.size(); i++) {
        uint32_t child_idx = _tags[i].index;
        std::vector<uint32_t> all_parents;
        uint32_t current = child_idx;
        
        // Walk up the direct parent chain
        while (_direct_parent_map.count(current) > 0) {
            current = _direct_parent_map[current];
            all_parents.push_back(current);
        }
        
        if (!all_parents.empty()) {
            _parent_map[child_idx] = all_parents;
        }
    }
    
    // Clear temporary data
    _direct_parent_map.clear();
    
    // Build flattened lookup table for O(1) queries
    for (const auto& [child_idx, parent_list] : _parent_map) {
        for (uint32_t parent_idx : parent_list) {
            uint64_t key = ((uint64_t)parent_idx << 32) | child_idx;
            _parent_lookup_table.insert(key);
        }
    }
    
    print_line(vformat("TagManager locked: %d tags, %d relationships", 
                       static_cast<uint64_t>(_tags.size()), static_cast<uint64_t>(_parent_lookup_table.size())));
}

uint32_t TagManager::get_tag_index(const StringName& p_tag_name) const
{
    auto it = _name_to_index_map.find(p_tag_name);
    return it != _name_to_index_map.end() ? _tags[it->second].index : INVALID_INDEX;
}

StringName TagManager::get_tag_name(uint32_t p_index) const
{
    return p_index < _tags.size() ? _tags[p_index].name : StringName();
}

bool TagManager::is_valid_tag(const StringName& p_tag_name) const
{
    return _name_to_index_map.find(p_tag_name) != _name_to_index_map.end();
}

bool TagManager::is_parent_of(uint32_t p_parent_index, uint32_t p_child_index) const
{
    // Exact match counts as parent
    if (p_parent_index == p_child_index) {
        return true;
    }
    
    // O(1) lookup in flattened table
    uint64_t key = ((uint64_t)p_parent_index << 32) | p_child_index;
    return _parent_lookup_table.count(key) > 0;
}

PackedStringArray TagManager::get_parent_tags(const StringName& p_tag_name) const
{
    PackedStringArray result;
    uint32_t index = get_tag_index(p_tag_name);
    
    if (index == INVALID_INDEX) {
        return result;
    }
    
    auto it = _parent_map.find(index);
    if (it != _parent_map.end()) {
        for (uint32_t parent_idx : it->second) {
            result.push_back(String(get_tag_name(parent_idx)));
        }
    }
    
    return result;
}

std::vector<uint32_t> TagManager::get_parent_indices(uint32_t p_index) const
{
    std::vector<uint32_t> result;
    
    if (p_index == INVALID_INDEX) {
        return result;
    }
    
    auto it = _parent_map.find(p_index);
    if (it != _parent_map.end()) result = it->second;
    
    return result;
}

PackedStringArray TagManager::get_all_tag_names() const
{
    PackedStringArray result;

    for (const Tag& tag : _tags) {
        result.push_back(String(tag.name));
    }

    return result;
}
