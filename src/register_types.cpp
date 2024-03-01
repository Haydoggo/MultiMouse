#include "register_types.h"

#include "raw_input.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

static RawInput* raw_input = nullptr;

void initialize_raw_input_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<RawInput>();
		raw_input = memnew(RawInput);
		Engine::get_singleton()->register_singleton("RawInput", raw_input);
	}
}

void uninitialize_raw_input_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		free(raw_input);
		Engine::get_singleton()->unregister_singleton("RawInput");
	}
}

extern "C"
{
	// Initialization.
	GDExtensionBool GDE_EXPORT raw_input_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_raw_input_module);
		init_obj.register_terminator(uninitialize_raw_input_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}