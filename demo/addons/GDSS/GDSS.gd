class_name GDSMLHighlight
extends EditorSyntaxHighlighter

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
