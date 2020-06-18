#include "gdpsengine.hpp"


#include "Parser.hpp"
#include "Compiler.hpp"
#include "PSEngine.hpp"
#include "gdLogger.hpp"
#include "EnumHelpers.hpp"

#include <map>


#include "core/print_string.h"
#include "core/os/file_access.h"


void GDPSEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_process"), &GDPSEngine::_process);
    ClassDB::bind_method(D_METHOD("load_game_from_file_path","fpath"), &GDPSEngine::load_game_from_file_path);
    ClassDB::bind_method(D_METHOD("get_level_state"), &GDPSEngine::get_level_state);
    ClassDB::bind_method(D_METHOD("get_turn_deltas"), &GDPSEngine::get_turn_deltas);
    ClassDB::bind_method(D_METHOD("send_input","input"), &GDPSEngine::send_input);
    ClassDB::bind_method(D_METHOD("get_level_count"), &GDPSEngine::get_level_count);
    ClassDB::bind_method(D_METHOD("load_level","level_idx"), &GDPSEngine::load_level);
    ClassDB::bind_method(D_METHOD("is_level_complete"), &GDPSEngine::is_level_complete);

}

GDPSEngine::GDPSEngine() {
}

GDPSEngine::~GDPSEngine() {
    // add your cleanup here
}

void GDPSEngine::_init() {
    shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());
    m_psengine = PSEngine(gdlogger);
}


void GDPSEngine::_process(float delta) {

  
}

void GDPSEngine::load_game_from_file_path(String p_fpath)
{
    shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());

    Error err;
	FileAccess *gdfile = FileAccess::open(p_fpath, FileAccess::READ, &err);

    std::string file_content = "";
    if(gdfile->is_open())
    {
        print_line("gdpsengine : opened gd file");
        file_content = gdfile->get_as_utf8_string().utf8();
        //print_line(file_content.c_str());
        
    }
    else
    {
        print_line("gdpsengine : did not open gd file");
    }

    optional<ParsedGame> parsed_game = Parser::parse_from_string(file_content,gdlogger);
    if(parsed_game.has_value())
    {
        print_line("gdpsengine : parsing complete,proceding to compilation") ;
        Compiler puzzle_compiler(gdlogger);
        std::optional<CompiledGame> compiled_puzzle_opt = puzzle_compiler.compile_game(parsed_game.value());
        if(!compiled_puzzle_opt.has_value())
        {
            print_line("gdpsengine : could not compile game properly"); 
        }
        else
        {
            print_line("gdpsengine : loading game"); 
            m_psengine.load_game(compiled_puzzle_opt.value());
        }
    }
    else
    {
        print_line("gdpsengine : something went wrong during the game parsing"); 
    }

    gdfile->close();
	memdelete(gdfile);
}

Dictionary GDPSEngine::get_level_state()
{
    PSEngine::Level level_state = m_psengine.get_level_state();
    Dictionary lvl;
    lvl["level_index"]= level_state.level_idx;
    lvl["width"]= level_state.width;
    lvl["height"]= level_state.height;

    Array cells;
    for(const PSEngine::Cell& cell: level_state.cells)
    {
        Dictionary cl;
        cl["x"] = cell.x;
        cl["y"] = cell.y;

        Array objects;
        for(const auto& pair: cell.objects)
        {
            objects.append(pair.first->identifier.c_str());
        }
        cl["objects"] = objects;

        cells.append(cl);
    }
    lvl["cells"] = cells;

    return lvl;
}

void GDPSEngine::send_input(String p_input)
{
    if(p_input == "undo")
    {
        m_psengine.undo();
    }
    else if (p_input == "restart")
    {
        m_psengine.restart_level();
    }
    else
    {
        std::map<String,PSEngine::InputType> input_mapping = {
            {"up", PSEngine::InputType::Up},
            {"left", PSEngine::InputType::Left},
            {"right", PSEngine::InputType::Right},
            {"down", PSEngine::InputType::Down},
            {"action", PSEngine::InputType::Action},
        };

        if( input_mapping.find(p_input) != input_mapping.end() )
        {
            m_psengine.receive_input(input_mapping[p_input]);
        }
        else
        {
            print_error("Incorrect input sent to the psengine");
        }
    }
    m_psengine.print_game_state();
}

Array GDPSEngine::get_turn_deltas()
{
    vector<PSEngine::SubturnHistory> ps_turn_deltas = m_psengine.get_turn_deltas();

    Array gd_turn_deltas;

    for(const auto& ps_subturn_delta : ps_turn_deltas)
    {
        Array gd_subturn_deltas;

        for(const auto& ps_rule_delta : ps_subturn_delta.steps)
        {
            Dictionary gd_rule_delta;
            gd_rule_delta["rule"] = ps_rule_delta.rule_applied.rule_line;
            gd_rule_delta["direction"] = enum_to_str(ps_rule_delta.rule_direction,PSEngine::to_absolute_direction).value_or("ERROR").c_str();
            gd_rule_delta["origin_x"] = ps_rule_delta.origin_x;
            gd_rule_delta["origin_y"] = ps_rule_delta.origin_y;

            Array gd_cells_deltas;

            for(const auto& ps_cell_delta : ps_rule_delta.cell_deltas)
            {
                Dictionary gd_cell_delta;
                gd_cell_delta["x"] = ps_cell_delta.x;
                gd_cell_delta["y"] = ps_cell_delta.y;

                Array gd_deltas;
                
                for(const auto& ps_delta : ps_cell_delta.deltas)
                {
                    Dictionary gd_delta;
                    //todo the pointer check shouldn't be necessary and probably means i need to fix something in psionic
                    gd_delta["object"] = ps_delta.object.get() != nullptr ? ps_delta.object->identifier.c_str() : "ERROR";
                    gd_delta["type"] = enum_to_str(ps_delta.type,PSEngine::to_object_delta_type).value_or("ERROR").c_str();

                    gd_deltas.append(gd_delta);
                }
                gd_cell_delta["deltas"] = gd_deltas;

                gd_cells_deltas.append(gd_cell_delta);
            }

            gd_rule_delta["cell_deltas"] = gd_cells_deltas;

            gd_subturn_deltas.append(gd_rule_delta);
        }

        gd_turn_deltas.append(gd_subturn_deltas);
    }

    return gd_turn_deltas;
}

int GDPSEngine::get_level_count()
{
    return m_psengine.get_number_of_levels();
}
    
void GDPSEngine::load_level(int p_level_idx)
{
    m_psengine.load_level(p_level_idx);
}

bool GDPSEngine::is_level_complete()
{
    return m_psengine.is_level_won();
}