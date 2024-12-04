#ifndef FLUID_REGISTER_TYPES_H
#define FLUID_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_fluid_module(ModuleInitializationLevel p_level);
void uninitialize_fluid_module(ModuleInitializationLevel p_level);

#endif