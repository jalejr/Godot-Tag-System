#include "tag_container.h"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "tag_manager.h"
#include <cstdint>
#include <algorithm>

void TagContainer::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("add_tags", "tags"), &TagContainer::add_tags);
    ClassDB::bind_method(D_METHOD("remove_tags", "tags"), &TagContainer::remove_tags);
    ClassDB::bind_method(D_METHOD("add_tag", "tag_name", "count"), &TagContainer::add_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("remove_tag", "tag_name", "count"), &TagContainer::remove_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("clear"), &TagContainer::clear);
    
    ClassDB::bind_method(D_METHOD("has_tag", "tag_name", "is_exact"), &TagContainer::has_tag, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_all_tags", "tags", "is_exact"), &TagContainer::has_all_tags, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_any_tags", "tags", "is_exact"), &TagContainer::has_any_tags, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_all_in_container", "other", "is_exact"), &TagContainer::has_all_in_container, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_any_in_container", "other", "is_exact"), &TagContainer::has_any_in_container, DEFVAL(false));
    
    ClassDB::bind_method(D_METHOD("watch_tag", "tag_name", "callable"), &TagContainer::watch_tag);
    ClassDB::bind_method(D_METHOD("unwatch_tag", "tag_name", "callable"), &TagContainer::unwatch_tag);

    ClassDB::bind_method(D_METHOD("get_tag_count", "tag_name"), &TagContainer::get_tag_count);
    ClassDB::bind_method(D_METHOD("get_num_tags"), &TagContainer::get_num_tags);
    ClassDB::bind_method(D_METHOD("get_tag_names"), &TagContainer::get_tag_names);

    ClassDB::bind_method(D_METHOD("serialize"), &TagContainer::serialize);
    ClassDB::bind_static_method("TagContainer", D_METHOD("deserialize", "bytes"), &TagContainer::deserialize);
}

void TagContainer::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY) _ready();
}

void TagContainer::_ready()
{
    _tag_manager = TagManager::get_singleton();
    ERR_FAIL_NULL_MSG(_tag_manager, "TagContainer created before TagManager is ready. Something really wrong happened...");
}

bool TagContainer::add_tag(const StringName& p_tag_name, int32_t p_count)
{
    ERR_FAIL_COND_V_MSG(p_count <= 0, false, vformat("Tag '%s' added with invalid count.", String(p_tag_name)));

    uint32_t index;
    if(!_resolve_index(p_tag_name, index)) return false;

    _tag_to_count_map[index] += p_count;

    if (_tag_to_count_map[index] == p_count) {
        std::vector<uint32_t> parents = _tag_manager->get_parent_indices(index);
        for (uint32_t parent_idx : parents) {
            _ancestor_to_count[parent_idx]++;
        }
    }

    _watcher.notify(p_tag_name, _tag_to_count_map[index], _tag_manager);

    return true;
}

void TagContainer::add_tags(const TypedArray<StringName>& p_tags)
{
    for (int i = 0; i < p_tags.size(); i++) {
        StringName tag = p_tags[i];
        if (tag != StringName()) add_tag(tag);
    }
}

bool TagContainer::remove_tag(const StringName& p_tag_name, int32_t p_count)
{
    ERR_FAIL_COND_V_MSG(p_count <= 0, false, vformat("Tag '%s' removed with invalid count.", String(p_tag_name)));

    uint32_t index;
    if (!_resolve_index(p_tag_name, index)) return false;

    auto it = _tag_to_count_map.find(index);
    if (it == _tag_to_count_map.end()) return false;

    it->second -= p_count;
    _watcher.notify(p_tag_name, std::max(0, it->second), _tag_manager);

    if (it->second <= 0) {
        _tag_to_count_map.erase(it);

        std::vector<uint32_t> parents = _tag_manager->get_parent_indices(index);
        for (uint32_t parent_idx : parents) {
            auto parent_it = _ancestor_to_count.find(parent_idx);
            if (parent_it == _ancestor_to_count.end()) continue;

            parent_it->second--;

            if(parent_it->second <= 0) {
                _ancestor_to_count.erase(parent_idx);
            }
        }
    }

    return true;
}

void TagContainer::remove_tags(const TypedArray<StringName>& p_tags)
{
    for (int i = 0; i < p_tags.size(); i++) {
        StringName tag = p_tags[i];
        if (tag != StringName()) remove_tag(tag);
    }
}

void TagContainer::clear()
{
    _tag_to_count_map.clear();
}

bool TagContainer::has_tag(const StringName& p_tag_name, bool p_is_exact) const
{
    uint32_t query_idx;
    if (!_resolve_index(p_tag_name, query_idx)) return false;

    return _has_tag_by_index(query_idx, p_is_exact);
}

bool TagContainer::has_all_tags(const TypedArray<StringName>& p_tags, bool p_is_exact) const
{
    if (p_tags.is_empty()) return true;

    for (int i = 0; i < p_tags.size(); i++) {
        uint32_t query_idx;
        if (!_resolve_index(StringName(p_tags[i]), query_idx)) return false;

        if (!_has_tag_by_index(query_idx, p_is_exact)) return false;
    }

    return true;
}

bool TagContainer::has_any_tags(const TypedArray<StringName>& p_tags, bool p_is_exact) const
{
    if (p_tags.is_empty()) return false;

    for (int i = 0; i < p_tags.size(); i++) {
        uint32_t query_idx;
        if (!_resolve_index(StringName(p_tags[i]), query_idx)) continue;

        if (_has_tag_by_index(query_idx, p_is_exact)) return true;
    }

    return false;
}

bool TagContainer::has_all_in_container(TagContainer* p_other, bool p_is_exact) const
{
    if (!p_other || p_other->get_num_tags() == 0) return true;

    for (const auto& [query_idx, _] : p_other->_tag_to_count_map) {
        if (!_has_tag_by_index(query_idx, p_is_exact)) return false;
    }

    return true;
}

bool TagContainer::has_any_in_container(TagContainer* p_other, bool p_is_exact) const
{
    if (!p_other || p_other->get_num_tags() == 0) return false;

    for (const auto& [query_idx, _] : p_other->_tag_to_count_map) {
        if (_has_tag_by_index(query_idx, p_is_exact)) return true;
    }

    return false;
}

int32_t TagContainer::get_tag_count(const StringName& p_tag_name) const
{
    uint32_t index;
    if (!_resolve_index(p_tag_name, index)) return 0;

    auto it = _tag_to_count_map.find(index);
    return (it != _tag_to_count_map.end()) ? it->second : 0;
}

void TagContainer::watch_tag(const StringName& p_tag_name, const Callable& p_callback)
{
    _watcher.watch(p_tag_name, p_callback, _tag_manager);
}

void TagContainer::unwatch_tag(const StringName& p_tag_name, const Callable& p_callback)
{
    _watcher.unwatch(p_tag_name, p_callback, _tag_manager);
}

PackedByteArray TagContainer::serialize() const
{
    PackedByteArray bytes;
    
    bytes.resize(4);
    bytes.encode_u32(0, _tag_to_count_map.size());
    
    for (const auto& [index, count] : _tag_to_count_map) {
        int offset = bytes.size();
        bytes.resize(offset + 8);
        bytes.encode_u32(offset, index);
        bytes.encode_s32(offset + 4, count);
    }
    
    return bytes;
}

TagContainer* TagContainer::deserialize(const PackedByteArray& p_bytes)
{
    TagContainer* container = memnew(TagContainer);
    
    if (p_bytes.size() < 4) { return container; }
    
    uint32_t count = p_bytes.decode_u32(0);
    int offset = 4;
    
    for (uint32_t i = 0; i < count; i++) {
        if (offset + 8 > p_bytes.size()) break;
        
        uint32_t key = p_bytes.decode_u32(offset);
        int32_t value = p_bytes.decode_s32(offset + 4);
        container->_tag_to_count_map[key] = value;
        
        offset += 8;
    }
    
    return container;
}

PackedStringArray TagContainer::get_tag_names() const
{
    PackedStringArray result;
    
    for (const auto& [index, count] : _tag_to_count_map) {
        if (count > 0) {
            result.push_back(String(_tag_manager->get_tag_name(index)));
        }
    }
    
    return result;
}

bool TagContainer::_resolve_index(const StringName& p_tag_name, uint32_t& r_index) const
{
    r_index = _tag_manager->get_tag_index(p_tag_name);

    if (r_index == TagManager::INVALID_INDEX) {
        ERR_PRINT(vformat("Tag '%s' not registered in TagManager", String(p_tag_name)));
        return false;
    }

    return true;
}

bool TagContainer::_has_tag_by_index(uint32_t p_query_idx, bool p_is_exact) const
{
    auto it = _tag_to_count_map.find(p_query_idx);
    if (it != _tag_to_count_map.end() && it->second > 0) return true;

    if (p_is_exact) return false;

    return _ancestor_to_count.find(p_query_idx) != _ancestor_to_count.end();
}