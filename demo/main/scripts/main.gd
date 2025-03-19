extends Control

func _ready():
	var gd = GDSML.new()
	gd.load_gdsml("res://test.gdsml", self)
