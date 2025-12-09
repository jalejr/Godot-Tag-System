@tool
extends EditorPlugin

var main_screen: Control
var needs_generation: bool = false

const MAIN_SCREEN_TAG_MANAGER = preload("uid://bbtty32bxsue2")

func _enter_tree() -> void:
	add_autoload_singleton("TagSystem", "res://addons/tag_system/scripts/tag_system_autoload.gd")
	
	main_screen = MAIN_SCREEN_TAG_MANAGER.instantiate()
	main_screen.tag_changed.connect(_on_tag_changed)

	EditorInterface.get_editor_main_screen().add_child(main_screen)
	_make_visible(false)


func _exit_tree():
	remove_autoload_singleton("TagSystem")
	
	if main_screen:
		
		if needs_generation:
			_generate_constants()
			needs_generation = false
		
		main_screen.queue_free()


func _has_main_screen():
	return true


func _make_visible(visible: bool) -> void:
	if main_screen:
		main_screen.visible = visible
		
	if not visible and needs_generation:
		_generate_constants()
		needs_generation = false


func _get_plugin_name():
	return "Tags"


func _get_plugin_icon():
	return EditorInterface.get_editor_theme().get_icon("Node", "EditorIcons")


func _on_tag_changed():
	if main_screen:
		main_screen.save_database()
		needs_generation = true


func _generate_constants():
	if not main_screen:
		return

	var database = main_screen.database
	if not database:
		return

	# Generate GDScript constants
	var gdscript_content = database.generate_gdscript_constants()
	var gdscript_path = "res://addons/tag_system/tags.gd"
	
	var gdscript_dir = gdscript_path.get_base_dir()
	if not DirAccess.dir_exists_absolute(gdscript_dir):
		var err = DirAccess.make_dir_recursive_absolute(gdscript_dir)
		if err != OK:
			push_error("Failed to create directory: " + gdscript_dir)
			return
	
	var file = FileAccess.open(gdscript_path, FileAccess.WRITE)
	if file:
		file.store_string(gdscript_content)
		file.close()
		print("Generated: ", gdscript_path)
	else:
		push_error("Failed to create file: " + gdscript_path + " (Error: " + str(FileAccess.get_open_error()) + ")")
	
	# Generate C++ header
	var cpp_content = database.generate_cpp_header()
	var cpp_path = "res://addons/tag_system/tags.h"
	
	var cpp_dir = cpp_path.get_base_dir()
	if not DirAccess.dir_exists_absolute(cpp_dir):
		var err = DirAccess.make_dir_recursive_absolute(cpp_dir)
		if err != OK:
			push_error("Failed to create directory: " + cpp_dir)
			return

	file = FileAccess.open(cpp_path, FileAccess.WRITE)
	if file:
		file.store_string(cpp_content)
		file.close()
		print("Generated: ", cpp_path)
	else:
		push_error("Failed to create file: " + gdscript_path + " (Error: " + str(FileAccess.get_open_error()) + ")")
	
	
	# Refresh filesystem
	EditorInterface.get_resource_filesystem().scan()
