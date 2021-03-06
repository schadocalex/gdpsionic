#ifndef GDPSENGINE_H
#define GDPSENGINE_H

#include "core/reference.h"
#include "core/dictionary.h"
#include "core/image.h"

#include <vector>
#include <string>
#include <map>

#include "PSEngine.hpp"
#include "CompiledGame.hpp"
#include "PSLogger.hpp"

class GDPSEngine: public Reference {
    GDCLASS(GDPSEngine, Reference)

private:
    PSEngine m_psengine;
    CompiledGame m_compiled_game;

    map<string,CompiledGame::ObjectGraphicData> m_cached_graphic_data;

    bool game_loaded = false;

public:
    static void _bind_methods();

    GDPSEngine();
    ~GDPSEngine();

    void _init(); // our initializer called by Godot

    void _process(float delta);

    void load_game_from_file_path(String p_fpath);

    Dictionary get_level_state();

    Array tick(float p_delta);
    Array send_input(String p_input);
    Array get_turn_deltas();

    int get_level_count();
    void load_level(int p_level_idx);

    bool is_level_complete();

    Ref<Image> get_texture_for_display();

protected:
    Array convert_turn_deltas(vector<PSEngine::SubturnHistory> p_turn_delta);

    vector<vector<string>> get_ordered_level_objects_by_collision_layers() const;
    void cache_graphic_data();

};

#endif
