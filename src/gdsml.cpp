#include "gdsml.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/gd_script.hpp>


using namespace godot;


// parse and show .gdsml files
void GDSML::load_gdsml(String gdsml_path, Node *root) {
    bool has_script = false;
    bool script_loaded = false;
    bool on_scene = false;

    Dictionary style;

    if (!FileAccess::file_exists(gdsml_path)) {
        UtilityFunctions::printerr(String("File '{0}' does not exist.").format(Array::make(gdsml_path)));
        return;
    }

    Dictionary classed;

    XMLParser *parser = memnew(XMLParser);
    if (parser->open(gdsml_path) != OK) {
        memdelete(parser);
        return;
    }

    PackedStringArray built_in = {
        "Scene",
        "Script"
    };

    Array signals = {};
    Array stack = {};
    stack.append(root);

    int index = 0;

    Node *current_node = nullptr;

    while (parser->read() != ERR_FILE_EOF) {
        if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
            index++;
            String name = parser->get_node_name();
            
            if (built_in.has(name)) {
                if (name == "Scene") {
                    on_scene = true;
                    for (int i = 0; i < parser->get_attribute_count(); i++) {
                        String att_name = parser->get_attribute_name(i);
                        if (att_name == "style") {
                            String style_path = parser->get_attribute_value(i);
                            if (FileAccess::file_exists(style_path)) {
                                style = parse_style(style_path);
                            } else {
                                UtilityFunctions::printerr(String("Style file '{0}' does not exist.").format(Array::make(style_path)));
                            }
                        }
                    }
                } else if (parser->get_node_name() == "Script") {
                    if (!has_script) {
                        has_script = true;
                    } else {
                        UtilityFunctions::printerr("Only one script tag can be present per GDSML scene file.");
                    }
                }
                continue;
            }

            if (on_scene) {
                Variant node_ = ClassDB::instantiate(name);
                current_node = Object::cast_to<Node>(node_);
    
                if (!current_node) {
                    continue;
                }
    
                for (int i = 0; i < parser->get_attribute_count(); i++) {
                    String att_name = parser->get_attribute_name(i);
    
                    if (att_name == "class") {
                        String cl = parser->get_attribute_value(i);
                        String current_class = current_node->get_class();
    
                        if (!style.is_empty() && style.has(cl)) {
                            if (style[cl].get("extends") != current_class) {
                                UtilityFunctions::printerr(String("Node of type '{0}' cannot have class '{1}' as it extends a different base class.").format(Array::make(current_class, style[cl].get("extends"))));
                            }
                        }
    
                        classed[current_node] = cl;
                        continue;
                    } else if (att_name == "script") {
                        String script = parser->get_attribute_value(i);
                        
                        if (!FileAccess::file_exists(script)) {
                            UtilityFunctions::printerr(String("Script file '{0}' was not found. Is the path correct?").format(Array::make(script)));
                            continue;
                        }

                        Ref<Resource> scr = ResourceLoader::get_singleton()->load(script);

                        if (scr == nullptr) {
                            UtilityFunctions::printerr(String("Script '{0}' does not exist. Is the path correct?").format(Array::make(script)));
                            continue;
                        }

                        current_node->set_script(scr);
                        continue;
                    } else if (att_name == "signal") {
                        String signal_str = parser->get_attribute_value(i);
                        PackedStringArray signal_parts = signal_str.split("=>");
                    
                        String signal_name = signal_parts[0].strip_edges();
                        PackedStringArray method_parts = signal_parts[1].split(":");
                        
                        if (method_parts.size() < 1) {
                            UtilityFunctions::printerr(String("Missing method name in signal: '{0}'").format(Array::make(signal_str)));
                            continue;
                        }
                    
                        String method_name = method_parts[0].strip_edges();
                        PackedStringArray arg_strings = method_parts.slice(1);
                    
                        Array arguments;
                        for (int j = 0; j < arg_strings.size(); j++) {
                            String arg_str = arg_strings[j].strip_edges();
                            Variant arg = UtilityFunctions::str_to_var(arg_str);
                            if (!arg) {
                                arg = arg_str;
                            }
                            arguments.append(arg);
                        }
                    
                        Dictionary signal_info;
                        signal_info["emitter"] = current_node;
                        signal_info["signal_name"] = signal_name;
                        signal_info["method_name"] = method_name;
                        signal_info["arguments"] = arguments;

                        //UtilityFunctions::print(arg_strings);
                        //UtilityFunctions::print(arguments);
                    
                        signals.append(signal_info);

                        continue;
                    }

                    if (node_.has_key(att_name)) {
                        Variant::Type type = ClassDB::class_get_property(node_, att_name).get_type();
                        Variant att_value = parser->get_attribute_value(i);

                        if (type != Variant::STRING) {
                            att_value = UtilityFunctions::str_to_var(parser->get_attribute_value(i));
    
                            if (!att_value) {
                                att_value = parser->get_attribute_value(i);
                            }
                        }

                        current_node->set(att_name, att_value);
                    } else {
                        UtilityFunctions::printerr(String("{0} does not have property '{1}'").format(Array::make(node_, att_name)));
                    }
                }
    
                if (!stack.is_empty()) {
                    Node *parent = Object::cast_to<Node>(stack[stack.size() - 1]);
    
                    if (parent) {
                        parent->add_child(current_node);
                    }
                }
    
                stack.append(node_);
            }
        } else if (parser->get_node_type() == XMLParser::NODE_TEXT) {
            if (has_script && !script_loaded) {
                String source_code = parser->get_node_data().dedent();
                Ref<GDScript> script = script_instance(source_code);
                if (script.is_valid()) {
                    root->set_script(script);
                }
                script_loaded = true;
            }

            if (current_node) {
                Variant r = stack[stack.size() - 1];

                if (r.has_key("text")) {
                    current_node->set("text", parser->get_node_data());
                }
            }
        } else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
            if (parser->get_node_name() == "Scene") {
                on_scene = false;
            }

            if (stack.size() > 1) {
                stack.pop_back();
            }
        }
    }
    memdelete(parser);

    if (!classed.is_empty() && !style.is_empty()) {
        for (int i = 0; i < classed.size(); i++) {
            String current_class = classed[Array(classed.keys())[i]];

            if (style.has(current_class)) {
                String id = Array(classed.keys())[i];
                Ref<RegEx> regex = memnew(RegEx);
                regex->compile(".+#(.+)>"); // get instance id
                id = regex->sub(id, "$1", true);

                Dictionary props = style[current_class].get("properties");
                Object *j = UtilityFunctions::instance_from_id(id.to_int());

                for (int prop = 0; prop < props.size(); prop++) {
                    String property = Array(props.keys())[prop];
                    Variant value = UtilityFunctions::str_to_var(props[Array(props.keys())[prop]]);
                    String class_n = j->get_class();

                    if (!value) {
                        value = props[Array(props.keys())[prop]];
                    }

                    j->set(property, value);
                }
            }
        }
    }

    if (!signals.is_empty()) {
        if (has_script) {
            for (int i = 0; i < signals.size(); i++) {
                Dictionary signal_info = signals[i];
                Node *emitter = Object::cast_to<Node>(signal_info["emitter"]);
                String signal_name = signal_info["signal_name"];
                String method_name = signal_info["method_name"];
                Array arguments = signal_info["arguments"];
    
                if (!emitter) {
                    UtilityFunctions::printerr("Invalid emitter.");
                    continue;
                }

                for (int i = 0; i < arguments.size(); i++) {
                    if (arguments[i] == "self") {
                        arguments[i] = emitter;
                    }
                }

                Callable callable = (arguments.is_empty()) ? Callable(root, method_name) : Callable(root, method_name).bindv(arguments);

                Error err = emitter->connect(signal_name, callable);
                if (err != OK) {
                    UtilityFunctions::printerr(String("Failed to connect signal '{0}' to '{1}'.").format(Array::make(signal_name, method_name)));
                }
            }
        }
    }
}


// parse .gdss (godot stylesheet)
Dictionary GDSML::parse_style(String gdss_path) {
    Ref<FileAccess> file = FileAccess::open(gdss_path, FileAccess::READ);
    String content = file->get_as_text();

    Ref<RegEx> comments = memnew(RegEx);

    // multi-line comments (/* and */)
    comments->compile("\\/\\*[\\s\\S]*?\\*\\/");
    content = comments->sub(content, "", true);

    // single-line comments (//)
    comments->compile("\\/\\/(.+)");
    content = comments->sub(content, "", true);

    Ref<RegEx> regex = memnew(RegEx); // main regex
    regex->compile("\\.(\\w+)(?:\\s+(\\w+))?\\s*\\{([^}]+)\\}");
    TypedArray<RegExMatch> matches = regex->search_all(content);
    Dictionary parsed;

    for (int i = 0; i < matches.size(); i++) {
        Ref<RegExMatch> m = matches[i];
        PackedStringArray r = m->get_strings();
        String class_name = r[1];
        String extending = r[2].strip_edges();

        if (extending.is_empty()) {
            UtilityFunctions::printerr(String("Class '{0}' in the style file is missing a base node type. Every class must specify what it extends.").format(Array::make(class_name)));
            return Dictionary();
        }

        PackedStringArray properties = r[3].strip_edges().split(";");
        Dictionary props;

        for (String prop : properties) {
            if (prop.is_empty()) continue;
            PackedStringArray kv = prop.strip_edges().split(":");
            String property = kv[0].strip_edges();
            String value = kv[kv.size() - 1].strip_edges();

            props[property] = value;
        }

        Dictionary info;
        info["extends"] = extending;
        info["properties"] = props;

        parsed[class_name] = info;
    }

    return parsed;
}


Ref<GDScript> GDSML::script_instance(String source_code) {
    Ref<GDScript> script = memnew(GDScript);
    script->set_source_code(source_code);

    if (script->reload() != OK) {
        UtilityFunctions::printerr("Failed to compile script.");
        return Ref<GDScript>();
    }

    return script;
}


void GDSML::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_gdsml", "gdsml_path", "root"), &GDSML::load_gdsml);
}