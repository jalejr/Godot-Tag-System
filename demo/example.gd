extends Node

func _ready() -> void:
	print("\n=== Tag System Test ===")
	print("Total tags registered: ", TagManager.get_tag_count())
	print("All tags: ", TagManager.get_all_tag_names())

	var container = TagContainer.new()
	container.add_tag(Tag.ABILITY_ATTACK_MELEE)

	print("\nContainer tests:")
	print("  Has 'Ability': ", container.has_tag(Tag.ABILITY))  # Should be true
	print("  Has 'Ability' exact: ", container.has_tag(Tag.ABILITY, true)) # Should be false
	print("  Added 'Ability' explicitly")
	container.add_tag(Tag.ABILITY)
	print("  Has 'Ability' exact: ", container.has_tag(Tag.ABILITY, true)) # Should be true
	print("  Has 'Ability.Attack': ", container.has_tag(Tag.ABILITY_ATTACK))  # Should be true
	print("  Has 'Ability.Attack.Melee': ", container.has_tag(Tag.ABILITY_ATTACK_MELEE))  # Should be true
	print("  Has 'Ability.Defend': ", container.has_tag(Tag.ABILITY_DEFEND))  # Should be false

	container.add_tag("Status.Buff.Strength", 5)
	print("\nStack test:")
	print("  Strength stacks: ", container.get_tag_count(Tag.STATUS_BUFF_STRENGTH))  # Should be 5
	
	var container2 = TagContainer.new()
	container2.add_tag(Tag.ABILITY_ATTACK)
	container2.add_tag(Tag.ABILITY)
	print("\nContainer comparison checks:")
	print("  Has all 'Ability.Attack': ", container.has_all(container2))  # Should be true
	print("  Has any 'Ability.Attack': ", container.has_any(container2))  # Should be true
	print("  Has all 'Ability.Attack' exact: ", container.has_all(container2, true))  # Should be false
	print("  Has any 'Ability.Attack' exact: ", container.has_any(container2, true))  # Should be true
	
	print("\nContainer watch checks:")
	container.watch_tag(Tag.STATUS, _test)
	print("  Is watching for: ", Tag.STATUS)
	print("  Adding: ", Tag.STATUS_BUFF_STRENGTH)
	container.add_tag(Tag.STATUS_BUFF_STRENGTH)
	container.unwatch_tag(Tag.STATUS, _test)
	print("  Stopped watching for: ", Tag.STATUS)
	print("  Adding: ", Tag.STATUS_BUFF_STRENGTH)
	container.add_tag(Tag.STATUS_BUFF_STRENGTH)
	container.watch_tag(Tag.STATUS_BUFF, _test)
	print("  Is watching for: ", Tag.STATUS_BUFF)
	print("  Adding: ", Tag.STATUS_BUFF_STRENGTH)
	container.add_tag(Tag.STATUS_BUFF_STRENGTH)
	print("  Adding: ", Tag.ABILITY_DEFEND)
	container.add_tag(Tag.ABILITY_DEFEND)
	container.watch_tag(Tag.ABILITY_DEFEND, _test)
	print("  Is watching for: ", Tag.ABILITY_DEFEND)
	print("  Adding: ", Tag.ABILITY_DEFEND)
	container.add_tag(Tag.ABILITY_DEFEND)
	
	print("\nContainer duplicate checks:")
	print("  Has tags: ", container.get_tag_names())
	var dup_container = container.duplicate()
	print("  Has tags: ", dup_container.get_tag_names())
	dup_container.add_tag(Tag.STATUS_BUFF)
	
	print("\nContainer serialize/deserialize checks:")
	print("  Has tags: ", container.get_tag_names())
	var bytes = container.serialize()
	print("  Serialized: ", bytes)
	var container_deserialized = TagContainer.deserialize(bytes)
	print("  Deserialized: ", container_deserialized.get_tag_names())
	print("\n=== Test Complete ===\n")


func _test(tag_name: StringName, tag_count: int):
	print("  Callback responding: %s : %d" % [tag_name, tag_count])
