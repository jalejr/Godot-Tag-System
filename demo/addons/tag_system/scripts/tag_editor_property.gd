@tool
extends EditorProperty

const MAIN_SCREEN_TAG_MANAGER = preload("uid://bbtty32bxsue2")

var _button := Button.new()
var _popup : Window

func _init():
	_button.pressed.connect(_on_button_pressed)
	add_child(_button)
	add_focusable(_button)


func _update_property() -> void:
	var current_text = str(get_edited_object()[get_edited_property()])
	_button.text = current_text if not current_text.is_empty() else "NO TAG PLEASE ADD"


func _on_button_pressed():
	if _popup != null:
		return
	
	_popup = Window.new()
	_popup.size = Vector2(1280, 720)
	
	var panel: Control = MAIN_SCREEN_TAG_MANAGER.instantiate()
	panel.tag_selected.connect(_on_tag_selected)
	panel.tag_changed.connect(_on_popup_tag_changed)
	_popup.add_child(panel)
	
	_popup.close_requested.connect(_on_popup_closed)
	
	EditorInterface.get_base_control().add_child(_popup)
	_popup.popup_centered()


func _on_popup_tag_changed():
	var editor_main = EditorInterface.get_editor_main_screen()
	for child in editor_main.get_children():
		if child.has_method("refresh_tree"):
			child.refresh_tree()
			child.generate_constants()


func _on_tag_selected(tag_path: String):
	emit_changed(get_edited_property(), tag_path)
	_button.text = tag_path
	_on_popup_closed()


func _on_popup_closed():
	if _popup == null: 
		return
	
	_popup.queue_free()
	_popup = null
