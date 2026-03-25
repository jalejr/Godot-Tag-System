#pragma once


#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "tag_manager.h"
#include "identity_hash.h"

using namespace godot;

class TagWatcher {
private:
    std::unordered_map<uint32_t, std::vector<Callable>, IdentityHash> _tag_to_callable_map;

public:
    TagWatcher() = default;

    void watch(const StringName& p_tag_name, const Callable& p_callback, TagManager* p_tag_manager);
    void unwatch(const StringName& p_tag_name, const Callable& p_callback, TagManager* p_tag_manager);
    void notify(const StringName& p_tag_name, int32_t p_count, TagManager* p_tag_manager);

private:
    void _fire(uint32_t p_notified_index, const StringName& p_tag_changed, int32_t p_count);
};