@tool
extends Control

signal tag_changed
signal tag_selected(tag_path: String)

const TAG_EDIT_DIALOG = preload("uid://bp3n14bgad0js")
const TAG_DELETE_DIALOG = preload("uid://bi14lfnuqd1sp")

@onready var search_box: LineEdit = %SearchBox
@onready var insert_box: LineEdit = %InsertBox
@onready var insert_button: Button = %InsertButton
@onready var tag_tree: Tree = %TagTree

var database: TagDatabase
var database_path: String = "res://addons/tag_system/data/tags.tres"
var edit_icon: Texture2D
var delete_icon: Texture2D

func _ready():
	var editor_theme = EditorInterface.get_editor_theme()
	edit_icon = editor_theme.get_icon("Edit", "EditorIcons")
	delete_icon = editor_theme.get_icon("Remove", "EditorIcons")
	
	search_box.text_changed.connect(_on_search_changed)
	insert_button.pressed.connect(_on_insert_pressed)
	insert_box.gui_input.connect(_on_insert_box_input)
	
	tag_tree.columns = 3
	tag_tree.set_column_title(0, "Tag Name")
	tag_tree.set_column_title(1, "Edit")
	tag_tree.set_column_title(2, "Delete")
	tag_tree.column_titles_visible = true
	tag_tree.hide_root = true
	
	tag_tree.set_column_expand(0, true)
	tag_tree.set_column_expand(1, false)
	tag_tree.set_column_expand(2, false)
	tag_tree.set_column_custom_minimum_width(1, 16)
	tag_tree.set_column_custom_minimum_width(2, 16)
	
	tag_tree.item_selected.connect(_on_item_selected)
	tag_tree.button_clicked.connect(_on_tree_button_clicked)
	
	load_database()


func load_database():
	if ResourceLoader.exists(database_path):
		database = load(database_path)
	else:
		database = TagDatabase.new()
		save_database()
	
	refresh_tree()


func save_database():
	if database:
		ResourceSaver.save(database, database_path)
		database.take_over_path(database_path)


func refresh_tree(filter: String = ""):
	tag_tree.clear()
	var root = tag_tree.create_item()
	
	if not database:
		return
	
	var all_tags = database.get_all_tags()
	var displayed_tags = {}
	
	for tag in all_tags:
		if filter.is_empty() or _tag_matches_filter(tag, filter):
			displayed_tags[tag] = true
			
			var parts = tag.split(".")
			
			for i in range(1, parts.size()):
				var ancestor = ".".join(parts.slice(0, i))
				displayed_tags[ancestor] = true
	
	var root_tags = database.get_root_tags()
	for tag in root_tags:
		if tag in displayed_tags:
			_add_tag_recursive(root, tag, displayed_tags)


func generate_constants():
	if not database:
		return
	
	var gdscript_content = database.generate_gdscript_constants()
	var file = FileAccess.open("res://addons/tag_system/tags.gd", FileAccess.WRITE)
	if file:
		file.store_string(gdscript_content)
		file.close()
	
	var cpp_content = database.generate_cpp_header()
	file = FileAccess.open("res://addons/tag_system/tags.h", FileAccess.WRITE)
	if file:
		file.store_string(cpp_content)
		file.close()
	
	EditorInterface.get_resource_filesystem().scan()


func _tag_matches_filter(tag: String, filter: String) -> bool:
	var lower_filter = filter.to_lower()
	
	for segment in tag.split("."):
		if segment.to_lower().contains(lower_filter):
			return true
	
	return false


func _add_tag_recursive(parent: TreeItem, tag_path: String, filter_dict: Dictionary):
	var segments = tag_path.split(".")
	var item = tag_tree.create_item(parent)
	
	var depth = tag_path.count(".")
	match depth:
		0: item.set_custom_color(0, Color.WHITE)
		1: item.set_custom_color(0, Color(0.8, 0.9, 1.0))
		2: item.set_custom_color(0, Color(0.6, 0.8, 1.0))
		_: item.set_custom_color(0, Color(0.4, 0.7, 1.0))
	
	# Column 0
	item.set_metadata(0, tag_path)
	item.set_text(0, segments[-1])
	
	var tooltip = "Full Name: " + tag_path
	var description = database.get_tag_description(tag_path)
	if not description.is_empty():
		tooltip += "\n\nDescription: " + description
	item.set_tooltip_text(0, tooltip)
	
	# Column 1
	item.set_cell_mode(1, TreeItem.CELL_MODE_ICON)
	item.add_button(1, edit_icon, 0, false, "Edit Description")
	
	# Column 2
	item.add_button(2, delete_icon, 1, false, "Delete tag")
	
	var children = database.get_children(tag_path)
	for child in children:
		if child in filter_dict:
			_add_tag_recursive(item, child, filter_dict)


func _on_item_selected():
	var item = tag_tree.get_selected()
	
	if not item : return
	
	var tag_path = item.get_metadata(0) as String
	tag_selected.emit(tag_path)


func _on_tree_button_clicked(item: TreeItem, _column: int, id: int, mouse_button_index: int):
	if mouse_button_index != MOUSE_BUTTON_LEFT:
		return
	
	var tag_path = item.get_metadata(0) as String
	
	match id:
		0:
			_show_edit_dialog(tag_path)
		1:
			_show_delete_dialog(tag_path)


func _show_edit_dialog(tag_path: String):
	var dialog: AcceptDialog = TAG_EDIT_DIALOG.instantiate()
	var label: Label = dialog.get_node("VBoxContainer/Label")
	label.text = "Tag: " + tag_path + "\nDescription:"
	var text_edit: TextEdit = dialog.get_node("VBoxContainer/TextEdit")

	text_edit.text = database.get_tag_description(tag_path)
	
	dialog.confirmed.connect(func():
		var new_description = text_edit.text.strip_edges()
		database.set_tag_description(tag_path, new_description)
		save_database()
		refresh_tree(search_box.text)
		dialog.queue_free()
	)
	
	dialog.canceled.connect(func():
		dialog.queue_free()
	)
	
	add_child(dialog)
	dialog.popup_centered()


func _show_delete_dialog(tag_path: String):
	var children = database.get_children(tag_path)
	var has_children = children.size() > 0
	var dialog: ConfirmationDialog = TAG_DELETE_DIALOG.instantiate()
	
	if has_children:
		dialog.dialog_text = "Delete '%s' and all %d children?\n\nThis cannot be undone!" % [tag_path, children.size()]
	else:
		dialog.dialog_text = "Delete '%s'?\n\nThis cannot be undone!" % tag_path
	
	dialog.confirmed.connect(func():
		var all_tags = database.get_all_tags()
		for tag in all_tags:
			if tag == tag_path or tag.begins_with(tag_path + "."):
				database.remove_tag(tag)
		
		save_database()
		tag_changed.emit()
		refresh_tree(search_box.text)
		dialog.queue_free()
	)
	
	dialog.canceled.connect(func():
		dialog.queue_free()
	)
	
	add_child(dialog)
	dialog.popup_centered()


func _on_search_changed(new_text: String):
	refresh_tree(new_text)


func _on_insert_pressed():
	var tag_path = insert_box.text.strip_edges()
	
	if tag_path.is_empty():
		_show_error("Please enter a taag path")
		return
	
	if not _is_valid_tag_path(tag_path):
		_show_error("Invalid tag path. Use format: Parent.Child.Subchild")
		return
	
	if database.has_tag(tag_path):
		_show_error("Tag already exists: " + tag_path)
		return
	
	database.add_tag(tag_path)
	save_database()
	tag_changed.emit()
	
	insert_box.clear()
	refresh_tree(search_box.text)
	
	print("Added tag: ", tag_path)


func _on_insert_box_input(event: InputEvent):
	if event is InputEventKey and event.pressed and event.keycode == KEY_ENTER:
		_on_insert_pressed()


func _is_valid_tag_path(path: String) -> bool:
	if path.is_empty():
		return false
	
	var regex = RegEx.new()
	regex.compile("^[A-Za-z0-9_.]+$")
	if not regex.search(path):
		return false
	
	if ".." in path:
		return false
	
	if path.begins_with(".") or path.ends_with("."):
		return false
	
	return true


func _show_error(message: String):
	var dialog = AcceptDialog.new()
	dialog.dialog_text = message
	add_child(dialog)
	dialog.popup_centered()
	dialog.confirmed.connect(func(): dialog.queue_free())
