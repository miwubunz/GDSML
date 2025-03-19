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


class GDSSHighlight extends EditorSyntaxHighlighter:
	func _get_name() -> String:
		return "GDSS"

	func _get_supported_languages() -> PackedStringArray:
		return ["gdss"]

	func _get_line_syntax_highlighting(line: int) -> Dictionary:
		var regex = RegEx.new()
		var edit = get_text_edit().get_line(line)
		var color_map = {}

		regex.compile("(?m)^\\s*([\\w\\/-]+)\\s*:\\s*(.+?)\\s*;.*")
		var n = regex.search_all(edit)

		if n.size() > 0:
			for i in n:
				var base = i.get_strings()
				var property = base[1]
				var value = base[2]

				var find1 = edit.find(property)
				var find2 = edit.find(value)

				if find1 > -1:
					color_map[find1] = { "color": Color.ANTIQUE_WHITE }
				if find2 > -1:
					color_map[find2] = { "color": Color.AQUAMARINE }

		var end_pos = edit.find(";")
		if end_pos > -1:
			color_map[end_pos] = { "color": Color.WHEAT }

		var comment_pos = edit.find("//")
		if comment_pos > -1:
			color_map[comment_pos] = { "color": Color(0.5,0.5,0.5,1) }

		regex.compile("\\.(\\w+)(?:\\s+(\\w+))?\\s*")
		n = regex.search_all(edit)

		if n.size() > 0:

			for i in n:
				var base = i.get_strings()
				var classname = base[1]
				var basenode = base[2]

				var find1 = edit.find(classname)
				var find2 = edit.find(basenode)
				if find1 > -1:
					color_map[find1] = { "color": Color.ANTIQUE_WHITE }
				if find2 > -1:
					color_map[find2] = { "color": Color.AQUAMARINE }

		var brace_open = edit.find("{")
		var brace_close = edit.find("}")

		if brace_open > -1:
			color_map[brace_open] = { "color": Color.CORNSILK }
		if brace_close > -1:
			color_map[brace_close] = { "color": Color.CORNSILK }

		return color_map


class GDSMLHighlight extends EditorSyntaxHighlighter:
	func _get_name() -> String:
		return "GDSML"

	func _get_supported_languages() -> PackedStringArray:
		return ["gdsml"]

	func _get_line_syntax_highlighting(line: int) -> Dictionary:
		var regex = RegEx.new()
		var color_map = {}
		var editor = get_text_edit().get_line(line)

		var index : int = 0

		regex.compile('(<!--.*?-->)|<\\/?(\\w+)|(\\w+)=|"([^"]*)"|(\\/>)|(?<=>)([^<]+)')
		var n = regex.search_all(editor)

		if !n.is_empty():
			for i in n:
				var base : PackedStringArray = i.get_strings()
				base.remove_at(0)
				var color

				match get_ind(base):
					0: color = Color(0.65, 0.65, 0.75, 1) # comments
					1: color = Color(0.5, 0.3, 1) # tags
					2: color = Color(0.3, 0.9, 0.6) # attributes
					3: color = Color(1, 0.6, 0.6) # values
					4: color = Color(0.4, 0.8, 1) # closing tags
					5: color = Color(0.95, 0.9, 0.85) # content

				var j = get_ind(base)
				if !j:
					continue
				var find = editor.find(base[j])
				
				if find > -1:
					color_map[find] = { "color": color }
					#color_map[find + i.get_string().length()] = { "color": Color.GRAY }
		index += 1

		return color_map


	func get_ind(items : Array):
		var index : int = 0
		for i in items:
			if i.is_empty():
				index += 1
				continue
			return index
