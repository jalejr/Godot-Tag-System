#include "tag_watcher.h"

#include <algorithm>

void TagWatcher::watch(const StringName& p_tag_name, const Callable& p_callback, TagManager* p_tag_manager)
{
    uint32_t index = p_tag_manager->get_tag_index(p_tag_name);
    
    if (index == TagManager::INVALID_INDEX) {
        ERR_PRINT(vformat("Tag Watcher: Tag '%s' not registered in TagManager", String(p_tag_name)));
        return;
    }

    _tag_to_callable_map[index].push_back(p_callback);
}

void TagWatcher::unwatch(const StringName& p_tag_name, const Callable& p_callback, TagManager* p_tag_manager)
{
    uint32_t index = p_tag_manager->get_tag_index(p_tag_name);

    if (index == TagManager::INVALID_INDEX) return;

    auto it = _tag_to_callable_map.find(index);
    if (it == _tag_to_callable_map.end()) return;

    std::vector<Callable>& callbacks = it->second;
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), p_callback), callbacks.end());

    if (callbacks.empty()) {
        _tag_to_callable_map.erase(it);
    }
}

void TagWatcher::notify(const StringName& p_tag_name, int32_t p_count, TagManager* p_tag_manager)
{
    uint32_t tag_index = p_tag_manager->get_tag_index(p_tag_name);

    if (tag_index != TagManager::INVALID_INDEX) {
        _fire(tag_index, p_tag_name, p_count);
    }

    // Fire on all ancestors
    PackedStringArray parents = p_tag_manager->get_parent_tags(p_tag_name);
    for (int i = 0; i < parents.size(); i++) {
        uint32_t parent_index = p_tag_manager->get_tag_index(parents[i]);
        if (parent_index != TagManager::INVALID_INDEX) {
            _fire(parent_index, p_tag_name, p_count);
        }
    }
}

void TagWatcher::_fire(uint32_t p_notified_index, const StringName& p_tag_changed, int32_t p_count)
{
    auto it = _tag_to_callable_map.find(p_notified_index);
    if (it == _tag_to_callable_map.end()) return;

    std::vector<Callable>& callbacks = it->second;
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < callbacks.size(); read_index++) {
        if (callbacks[read_index].is_valid()) {
            callbacks[read_index].call(Variant(StringName(p_tag_changed)), Variant(int32_t(p_count)));
            if (write_index != read_index) {
                callbacks[write_index] = callbacks[read_index];
            }
            write_index++;
        }
    }
    callbacks.resize(write_index);
}
