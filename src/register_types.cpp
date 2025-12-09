#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "tag_container.h"
#include "tag_database.h"
#include "tag_manager.h"

using namespace godot;

static TagManager *tag_manager_singleton = nullptr;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<TagManager>();
	tag_manager_singleton = memnew(TagManager);
	Engine::get_singleton()->register_singleton("TagManager", TagManager::get_singleton());
	GDREGISTER_CLASS(TagContainer)
	GDREGISTER_CLASS(TagDatabase)

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->unregister_singleton("TagManager");
	memdelete(tag_manager_singleton);
	tag_manager_singleton = nullptr;
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT tag_system_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
