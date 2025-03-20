class_name GDSSHighlight
extends EditorSyntaxHighlighter

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
