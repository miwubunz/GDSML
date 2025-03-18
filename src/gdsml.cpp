#include "gdsml.h"
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>


using namespace godot;


void GDSML::load_gdsml(String gdsml_path, Node *root) {
    // check for style file
    Dictionary style;
    String ex = gdsml_path.get_extension();
    String dir = gdsml_path.replace(ex, "gdss");

    if (FileAccess::file_exists(dir)) {
        //UtilityFunctions::print("style exists!!");
        style = parse_style(dir);
    }

    Dictionary classed;

    /*
    String base = gdsml_path.get_base_dir();
    String file = gdsml_path.get_file();
    String ex = gdsml_path.get_extension();
    String nm = file.replace(ex, "");
    String dir = (base.ends_with("/")) ? String("{0}{1}") : String("{0}/{1}");
    dir = dir.format(Array::make(base, nm + "gdss"));
    */


    XMLParser *parser = memnew(XMLParser);
    parser->open(gdsml_path);

    PackedStringArray built_in = {
        "Scene",
    };

    Array stack = {};
    stack.append(root);

    Node *current_node = nullptr;

    while (parser->read() != ERR_FILE_EOF) {
        //UtilityFunctions::print(stack);
        if (parser->get_current_line() < 1 && parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name() != "Scene") {
            UtilityFunctions::printerr("Error: GDSML scenes must be wrapped around a <Scene> tag. Ensure your root element is <Scene>.");
            memdelete(parser);
            return;
        }

        if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
            String name = parser->get_node_name();
            
            if (built_in.has(name)) {
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
                            UtilityFunctions::printerr(String("Error: Node of type '{0}' cannot have class '{1}' as it extends a different base class.").format(Array::make(current_class, style[cl].get("extends"))));
                        }
                    }

                    classed[current_node] = cl;
                    continue;
                }

                Variant att_value = UtilityFunctions::str_to_var(parser->get_attribute_value(i));

                if (!att_value) {
                    att_value = parser->get_attribute_value(i);
                    //UtilityFunctions::printerr(String("variable '{0}' couldnt be recognised, using a string instead...").format(Array::make(att_value)));
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
                //Variant temp = ClassDB::instantiate(cl);
                //UtilityFunctions::print(temp);

                if (r.has_key("text")) {
                    current_node->set("text", parser->get_node_data());
                }
            }
        } else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
            if (stack.size() > 1) {
                stack.pop_back();
            }
        }
    //UtilityFunctions::print(stack);
    }
    memdelete(parser);

    if (!classed.is_empty() && !style.is_empty()) {
        for (int i = 0; i < classed.size(); i++) {
            String current_class = classed[Array(classed.keys())[i]];
            if (style.has(current_class)) {
                String id = Array(classed.keys())[i];
                Ref<RegEx> regex = memnew(RegEx);
                regex->compile(".+#(.+)>");
                id = regex->sub(id, "$1", true);

                Dictionary props = style[current_class].get("properties");

                //UtilityFunctions::print(id);
                Object *j = UtilityFunctions::instance_from_id(id.to_int());
                for (int prop = 0; prop < props.size(); prop++) {
                    String property = Array(props.keys())[prop];
                    String value = UtilityFunctions::str_to_var(props[Array(props.keys())[prop]]);
                    String class_n = j->get_class();

                    if (!value) {
                        value = props[Array(props.keys())[prop]];
                    }

                    //UtilityFunctions::print(Array::make(property, value));

                    j->set(property, value);

                    //Variant k = ClassDB::instantiate(class_n);

                    //UtilityFunctions::print(j->get_property_list()[0]);
                    
                    /*
                    if () {
                        UtilityFunctions::print(property);
                    }
                    */
                }
                //UtilityFunctions::print(j);
            }
        }

        /*
        for (int i = 0; i < classed.size(); i++) {
            for (int j = 0; j < style.size(); j++) {
                if (style[Array(style.keys())[j]] == classed[Array(classed.keys())[i]]) {
                    UtilityFunctions::print(String("{0} matches class with {1}!!").format(Array::make(style[Array(style.keys())][j]), classed[Array(classed.keys())[i]]));
                }
            }
        }
        */
    }
}


Dictionary GDSML::parse_style(String gdss_path) {
    Ref<FileAccess> file = FileAccess::open(gdss_path, FileAccess::READ);
    String content = file->get_as_text();

    Ref<RegEx> regex = memnew(RegEx);
    regex->compile("\\.(\\w+)(?:\\s+(\\w+))?\\s*\\{([^}]+)\\}");
    TypedArray<RegExMatch> matches = regex->search_all(content);
    Dictionary parsed;
    //String r = matches;
    //PackedStringArray params = r.split(";");
    for (int i = 0; i < matches.size(); i++) {
        Ref<RegExMatch> m = matches[i];
        PackedStringArray r = m->get_strings();
        String class_name = r[1];
        String extending = r[2].strip_edges();
        if (extending.is_empty()) {
            UtilityFunctions::printerr(String("Error: Class '{0}' in the style file is missing a base node type. Every class must specify what it extends.").format(Array::make(class_name)));
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