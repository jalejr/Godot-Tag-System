#include "tag_container.h"
#include "godot_cpp/variant/dictionary.hpp"
#include "tag_manager.h"
#include <cstdint>
#include <vector>
#include <algorithm>

void TagContainer::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("add_tag", "tag_name", "count"), &TagContainer::add_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("remove_tag", "tag_name", "count"), &TagContainer::remove_tag, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("clear"), &TagContainer::clear);
    
    ClassDB::bind_method(D_METHOD("has_tag", "tag_name", "is_exact"), &TagContainer::has_tag, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_all", "other", "is_exact"), &TagContainer::has_all, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_any", "other", "is_exact"), &TagContainer::has_any, DEFVAL(false));
    
    ClassDB::bind_method(D_METHOD("watch_tag", "tag_name", "callable"), &TagContainer::watch_tag);
    ClassDB::bind_method(D_METHOD("unwatch_tag", "tag_name", "callable"), &TagContainer::unwatch_tag);

    ClassDB::bind_method(D_METHOD("get_tag_count", "tag_name"), &TagContainer::get_tag_count);
    ClassDB::bind_method(D_METHOD("get_num_tags"), &TagContainer::get_num_tags);
    ClassDB::bind_method(D_METHOD("get_tag_names"), &TagContainer::get_tag_names);

    ClassDB::bind_method(D_METHOD("serialize"), &TagContainer::serialize);
    ClassDB::bind_static_method("TagContainer", D_METHOD("deserialize", "bytes"), &TagContainer::deserialize);
}


bool TagContainer::add_tag(const StringName& p_tag_name, int32_t p_count)
{
    if (p_count <= 0) {
        ERR_PRINT(vformat("Tag '%s' was added an invalid count", String(p_tag_name)));
        return false;
    }
    
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) {
        ERR_PRINT("TagManager not initialized");
        return false;
    }
    
    uint32_t index = mgr->get_tag_index(p_tag_name);
    if (index != TagManager::INVALID_INDEX) {
        _tag_to_count_map[index] += p_count;  // Creates entry if doesn't exist, adds to existing
        int32_t count = _tag_to_count_map[index];
        _notify_watchers(p_tag_name, count);
        return true;
    } else {
        ERR_PRINT(vformat("Tag '%s' not registered in TagManager", String(p_tag_name)));
    }

    return false;
}

bool TagContainer::remove_tag(const StringName& p_tag_name, int32_t p_count)
{
    if (p_count <= 0) {
        ERR_PRINT(vformat("Tag '%s' was removed an invalid count", String(p_tag_name)));
        return false;
    }
    
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) {
        ERR_PRINT("TagManager not initialized");
        return false;
    }
    
    uint32_t index = mgr->get_tag_index(p_tag_name);
    if (index == TagManager::INVALID_INDEX) {
        return false;
    }
    
    auto it = _tag_to_count_map.find(index);
    if (it == _tag_to_count_map.end()) {
        return false;  // Tag not present
    }
    
    // Decrease count, remove if reaches 0
    it->second -= p_count;

    _notify_watchers(p_tag_name, std::max(0, it->second));

    if (it->second <= 0) {
        _tag_to_count_map.erase(it);
    }

    return true;
}

void TagContainer::clear()
{
    _tag_to_count_map.clear();
}

bool TagContainer::has_tag(const StringName& p_tag_name, bool p_is_exact) const
{
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return false;
    
    uint32_t query_idx = mgr->get_tag_index(p_tag_name);
    if (query_idx == TagManager::INVALID_INDEX) {
        return false;
    }
    
    // Check exact match first - count > 0
    auto it = _tag_to_count_map.find(query_idx);
    if (it != _tag_to_count_map.end() && it->second > 0) {
        return true;
    }

    if (p_is_exact) {
        return false;
    }
    
    // Check hierarchical match - is query a parent of any explicit tag?
    for (const auto& [tag_idx, count] : _tag_to_count_map) {
        if (count > 0 && mgr->is_parent_of(query_idx, tag_idx)) {
            return true;
        }
    }
    
    return false;
}

bool TagContainer::has_all(const Ref<TagContainer>& p_other, bool p_is_exact) const
{
    if (!p_other.is_valid() || p_other->get_num_tags() == 0) {
        return true;
    }
    
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return false;
    
    for (const auto& [query_idx, _] : p_other->_tag_to_count_map) {
        // Check exact match
        auto it = _tag_to_count_map.find(query_idx);
        if (it != _tag_to_count_map.end() && it->second > 0) {
            continue;
        }

        if (p_is_exact) {
            return false;
        }
        
        // Check hierarchical match
        bool found = false;
        for (const auto& [tag_idx, count] : _tag_to_count_map) {
            if (count > 0 && mgr->is_parent_of(query_idx, tag_idx)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return false;
        }
    }
    
    return true;
}

bool TagContainer::has_any(const Ref<TagContainer>& p_other, bool p_is_exact) const
{
    if (!p_other.is_valid() || p_other->get_num_tags() == 0) {
        return false;
    }
    
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return false;
    
    for (const auto& [query_idx, _] : p_other->_tag_to_count_map) {
        // Check exact match
        auto it = _tag_to_count_map.find(query_idx);
        if (it != _tag_to_count_map.end() && it->second > 0) {
            return true;  // Early exit on match
        }
        
        if (p_is_exact) {
            continue;
        }
        
        // Check hierarchical
        for (const auto& [tag_idx, count] : _tag_to_count_map) {
            if (count > 0 && mgr->is_parent_of(query_idx, tag_idx)) {
                return true;  // Early exit on match
            }
        }
    }
    
    return false;
}

int32_t TagContainer::get_tag_count(const StringName& p_tag_name) const
{
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return 0;
    
    uint32_t index = mgr->get_tag_index(p_tag_name);
    if (index == TagManager::INVALID_INDEX) {
        return 0;
    }
    
    auto it = _tag_to_count_map.find(index);
    return (it != _tag_to_count_map.end()) ? it->second : 0;
}

void TagContainer::watch_tag(const StringName& p_tag_name, const Callable& p_callback)
{
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return;

    uint32_t tag_index = mgr->get_tag_index(p_tag_name);
    std::vector<Callable>& callbacks = _tag_to_callable_map[tag_index];

    _tag_to_callable_map[tag_index].push_back(p_callback);
}

void TagContainer::unwatch_tag(const StringName& p_tag_name, const Callable& p_callback)
{
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return;

    uint32_t tag_index = mgr->get_tag_index(p_tag_name);
    
    auto it = _tag_to_callable_map.find(tag_index);
    if (it != _tag_to_callable_map.end()) {
        std::vector<Callable>& callbacks = it->second;
        
        callbacks.erase(
            std::remove(callbacks.begin(), callbacks.end(), p_callback),
            callbacks.end()
        );
        
        if (callbacks.empty()) {
            _tag_to_callable_map.erase(it);
        }
    }
}

PackedByteArray TagContainer::serialize() const
{
    PackedByteArray bytes;
    
    bytes.resize(4);
    bytes.encode_u32(0, _tag_to_count_map.size());
    
    for (const auto &pair : _tag_to_count_map) {
        int offset = bytes.size();
        bytes.resize(offset + 8);
        bytes.encode_u32(offset, pair.first);
        bytes.encode_s32(offset + 4, pair.second);
    }
    
    return bytes;
}

Ref<TagContainer> TagContainer::deserialize(const PackedByteArray& p_bytes)
{
    Ref<TagContainer> container = memnew(TagContainer);
    
    if (p_bytes.size() < 4) {
        return container;
    }
    
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
    TagManager* mgr = TagManager::get_singleton();
    if (!mgr) return result;
    
    for (const auto& [index, count] : _tag_to_count_map) {
        if (count > 0) {
            result.push_back(String(mgr->get_tag_name(index)));
        }
    }
    
    return result;
}

void TagContainer::_get_property_list(List<PropertyInfo> *p_list) const
{
    p_list->push_back(PropertyInfo(
        Variant::DICTIONARY,
        "_tag_to_count_map",
        PROPERTY_HINT_NONE,
        "_tag_to_count",
        PROPERTY_USAGE_STORAGE
    ));
    p_list->push_back(PropertyInfo(
        Variant::DICTIONARY,
        "_tag_to_callable_map",
        PROPERTY_HINT_NONE,
        "_tag_to_callable",
        PROPERTY_USAGE_NONE
    ));
}

bool TagContainer::_get(const StringName &p_name, Variant &r_ret) const
{
    if (p_name == StringName("_tag_to_count_map")) {
        Dictionary dict;

        for (const auto &tag_to_count : _tag_to_count_map) {
            dict[tag_to_count.first] = tag_to_count.second;
        }

        r_ret = dict;
        return true;
    }
    else if (p_name == StringName("_tag_to_callable_map")) {
        Dictionary dict;

        for (const auto &tag_to_callable : _tag_to_callable_map) {
            Array callable_array;
            for(const Callable &callable : tag_to_callable.second) {
                callable_array.push_back(callable);
            }
            dict[tag_to_callable.first] = callable_array;
        }

        r_ret = dict;
        return true;
    }

    return false;
}

bool TagContainer::_set(const StringName &p_name, const Variant &p_value)
{
    if (p_name == StringName("_tag_to_count_map")) {
        _tag_to_count_map.clear();

        Dictionary dict = p_value;
        Array keys = dict.keys();

        for (int i = 0; i < keys.size(); i++) {
            uint32_t key = keys[i];
            _tag_to_count_map[key] = dict[key];
        }

        return true;
    }
    else if (p_name == StringName("_tag_to_callable_map")) {
        _tag_to_callable_map.clear();

        Dictionary dict = p_value;
        Array keys = dict.keys();

        for (int i = 0; i < keys.size(); i++) {
            uint32_t key = keys[i];
            Array callable_array = dict[key];
            std::vector<Callable> callbacks;

            for (int j = 0; j < callable_array.size(); j++) {
                callbacks.push_back(callable_array[j]);
            }

            _tag_to_callable_map[key] = callbacks;
        }

        return true;
    }

    return false;
}

void TagContainer::_notify_watchers(const StringName& p_tag_name, int32_t p_count) {
    TagManager* mgr = TagManager::get_singleton();
    if(!mgr) return;
    PackedStringArray parents = mgr->get_parent_tags(p_tag_name);

    _notify_watcher(p_tag_name, p_tag_name, p_count); // call on exact
    
    for (int i = 0; i < parents.size(); i++) {
        _notify_watcher(parents[i], p_tag_name, p_count);
    }
}

void TagContainer::_notify_watcher(const StringName& p_tag_notified, const StringName& p_tag_changed, int32_t p_count) {
    TagManager* mgr = TagManager::get_singleton();
    if(!mgr) return;

    uint32_t notified_index = mgr->get_tag_index(p_tag_notified);

    auto it = _tag_to_callable_map.find(notified_index);
    if (it != _tag_to_callable_map.end()) {
        std::vector<Callable>& callbacks = it->second;
        size_t write_index = 0;

        for (size_t read_index = 0; read_index < callbacks.size(); read_index++) {
            if(callbacks[read_index].is_valid()) {
                callbacks[read_index].call(Variant(StringName(p_tag_changed)), Variant(int32_t(p_count)));
                if(write_index != read_index) {
                    callbacks[write_index] = callbacks[read_index];
                }
                write_index++;
            }
        }

        callbacks.resize(write_index); // lazy removal of invalid callbacks
    }
}
