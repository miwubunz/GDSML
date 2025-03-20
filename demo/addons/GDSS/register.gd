@tool
extends EditorPlugin


var GDSS : GDSSHighlight
var GDSMLH : GDSMLHighlight


func _enter_tree() -> void:
	GDSS = GDSSHighlight.new()
	GDSMLH = GDSMLHighlight.new()
	var editor = EditorInterface.get_script_editor()
	editor.register_syntax_highlighter(GDSS)
	editor.register_syntax_highlighter(GDSMLH)

	const setting_name = "docks/filesystem/textfile_extensions"
	var stuff : String = EditorInterface.get_editor_settings().get_setting(setting_name)
	stuff += "gdsml,gdss" if stuff.ends_with(",") else ",gdsml,gdss"
	#print(stuff)
	EditorInterface.get_editor_settings().set_setting(setting_name, stuff)


func _exit_tree() -> void:
	if is_instance_valid(GDSS):
		var editor = EditorInterface.get_script_editor()
		editor.unregister_syntax_highlighter(GDSS)
		editor.unregister_syntax_highlighter(GDSMLH)
		GDSS = null
		GDSMLH = null

	const setting_name = "docks/filesystem/textfile_extensions"
	var editor_settings = EditorSettings.new()
	var stuff = EditorInterface.get_editor_settings().get_setting(setting_name)
	var remover = RegEx.new()
	remover.compile(",gdss|gdss|,gdsml|gdsml")
	stuff = remover.sub(stuff, "", true)
	EditorInterface.get_editor_settings().set(setting_name, stuff)
