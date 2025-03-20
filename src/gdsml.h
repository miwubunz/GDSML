#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/gd_script.hpp>

using namespace godot;

class GDSML : public Object {
    GDCLASS(GDSML, Object);

protected:
    static void _bind_methods();

public:
    void load_gdsml(String gdsml_path, Node *root);
    Dictionary parse_style(String gdss_path);
    Ref<GDScript> script_instance(String source_code);
};