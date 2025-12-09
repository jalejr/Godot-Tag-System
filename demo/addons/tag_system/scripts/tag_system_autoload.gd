extends Node

# Autoload is to force tags to exist in game instance when loading in
func _ready() -> void:
	print("Initializing Tag System...")
	
	var database_path = "res://addons/tag_system/data/tags.tres"
	
	if not ResourceLoader.exists(database_path):
		push_error("TagDatabase not found at: " + database_path)
		push_error("Try creating tags to force creation and restarting editor")
		return
	
	var database = load(database_path) as TagDatabase
	if not database:
		push_error("Failed to load TagDatabase")
		return
	
	database.register_with_manager()
	print("Tag System initialized: %d tags registered" % TagManager.get_tag_count())
