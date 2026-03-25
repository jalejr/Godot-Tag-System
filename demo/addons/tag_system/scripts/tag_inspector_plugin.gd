@tool
class_name TagInspectorPlugin
extends EditorInspectorPlugin

const TagEditorProperty = preload("uid://bihibw11smhwu")

func _can_handle(object):
	return true


func _parse_property(object, type, name, hint_type, hint_string, usage_flags, wide):
	if type == TYPE_STRING_NAME and hint_string == "tag":
		add_property_editor(name, TagEditorProperty.new())
		return true
	return false
