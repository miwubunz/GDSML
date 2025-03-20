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


using namespace godot;


// parse and show .gdsml files
void GDSML::load_gdsml(String gdsml_path, Node *root) {
    // check for style file (.gdss) (OLD)
    /*
    Dictionary style;
    String ex = gdsml_path.get_extension();
    String dir = gdsml_path.replace(ex, "gdss");

    if (FileAccess::file_exists(dir)) {
        style = parse_style(dir);
    }
    */

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
    };

    Array stack = {};
    stack.append(root);

    int index = 0;

    Node *current_node = nullptr;

    while (parser->read() != ERR_FILE_EOF) {
        if (index < 1 && parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name() != "Scene") {
            UtilityFunctions::printerr("GDSML scenes must be wrapped around a <Scene> tag. Ensure your root element is <Scene>.");
            memdelete(parser);
            return;
        }

        if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
            index++;
            String name = parser->get_node_name();
            
            if (built_in.has(name)) {
                if (name == "Scene") {
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
                }
                continue;
            }

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
                }

                Variant att_value = UtilityFunctions::str_to_var(parser->get_attribute_value(i));

                if (!att_value) {
                    att_value = parser->get_attribute_value(i);
                }

                if (node_.has_key(att_name)) {
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
        } else if (parser->get_node_type() == XMLParser::NODE_TEXT) {
            if (current_node) {
                Variant r = stack[stack.size() - 1];

                if (r.has_key("text")) {
                    current_node->set("text", parser->get_node_data());
                }
            }
        } else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
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


void GDSML::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_gdsml", "gdsml_path", "root"), &GDSML::load_gdsml);
}