#pragma comment(lib, "user32.lib")
#include "raw_input.h"
#include <map>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/templates/rb_map.hpp>

using namespace godot;

RawInput *RawInput::singleton = nullptr;

static WNDPROC g_oldWndProc = nullptr;
static RBMap<HANDLE, MouseInfo *> mouseInfos;
static RBMap<HANDLE, int> keyboardIds;
static List<Vector2> mousePositions;

void RawInput::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("warp_mouse", "position", "id"), &RawInput::warp_mouse);
	ClassDB::bind_method(D_METHOD("is_mouse_button_pressed", "button", "id"), &RawInput::is_mouse_button_pressed);
	ClassDB::bind_method(D_METHOD("get_mouse_position", "id"), &RawInput::get_mouse_position);
	ClassDB::bind_method(D_METHOD("get_mouse_device_count"), &RawInput::get_mouse_device_count);
	ClassDB::bind_method(D_METHOD("initialize"), &RawInput::initialize);
	ADD_SIGNAL(MethodInfo("input_event", PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_RESOURCE_TYPE, "InputEvent")));
}

RawInput::RawInput()
{
	singleton = this;
	call_deferred("initialize");
}

RawInput::~RawInput()
{
	HWND handle = (HWND)DisplayServer::get_singleton()->window_get_native_handle(DisplayServer::WINDOW_HANDLE);
	SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR)g_oldWndProc);
}

void RawInput::initialize()
{
	HWND handle = (HWND)DisplayServer::get_singleton()->window_get_native_handle(DisplayServer::WINDOW_HANDLE);
	g_oldWndProc = (WNDPROC)GetWindowLongPtr(handle, GWLP_WNDPROC);
	SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR)MyWndProc);
	RAWINPUTDEVICELIST *devices;
	
	// Populate MiceInfos
	UINT numDevices;
	if (GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		UtilityFunctions::printerr("GetRawInputDeviceList failed");
		return;
	}
	devices = (RAWINPUTDEVICELIST *)malloc(sizeof(RAWINPUTDEVICELIST) * numDevices);
	if (GetRawInputDeviceList(devices, &numDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		UtilityFunctions::printerr("GetRawInputDeviceList failed");
		return;
	}
	for (UINT i = 0; i < numDevices; i++)
	{
		if (devices[i].dwType == RIM_TYPEMOUSE)
		{
			MouseInfo *mi = new MouseInfo;
			mi->id = mouseInfos.size();
			mouseInfos.insert(devices[i].hDevice, mi);
		}
	}
	std::free(devices);
}

MouseInfo *get_mouse_info(int id)
{
	MouseInfo *mi = nullptr;
	for (RBMap<HANDLE, MouseInfo *>::Element *E = mouseInfos.front(); E; E = E->next())
	{
		if (E->value()->id == id)
		{
			mi = E->value();
			return mi;
		}
	}
	if (!mi)
	{
		return nullptr;
	}
}

void RawInput::warp_mouse(Vector2 position, int id)
{
	MouseInfo *mi = get_mouse_info(id);
	if (!mi)
		return;
	mi->position = position;
}

bool RawInput::is_mouse_button_pressed(MouseButton button, int id)
{
	MouseInfo *mi = get_mouse_info(id);
	if (!mi)
		return false;
	switch (button)
	{
	case MouseButton::MOUSE_BUTTON_LEFT:
		return mi->leftButtonDown;

	case MouseButton::MOUSE_BUTTON_RIGHT:
		return mi->rightButtonDown;

	case MouseButton::MOUSE_BUTTON_MIDDLE:
		return mi->middleButtonDown;

	default:
		return false;
	}
}

Vector2 RawInput::get_mouse_position(int id)
{
	MouseInfo *mi = get_mouse_info(id);
	if (!mi)
		return Vector2();
	return mi->position;
}

int RawInput::get_mouse_device_count()
{
	int deviceCount = 0;
	RAWINPUTDEVICELIST *devices;
	UINT numDevices;
	if (GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		UtilityFunctions::printerr("GetRawInputDeviceList failed");
		return 0;
	}
	devices = (RAWINPUTDEVICELIST *)malloc(sizeof(RAWINPUTDEVICELIST) * numDevices);
	if (GetRawInputDeviceList(devices, &numDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		UtilityFunctions::printerr("GetRawInputDeviceList failed");
		return 0;
	}
	for (UINT i = 0; i < numDevices; i++)
	{
		if (devices[i].dwType == RIM_TYPEMOUSE)
		{
			deviceCount++;
		}
	}
	std::free(devices);
	return deviceCount;
}

void handle_mouse(RAWMOUSE rm, MouseInfo *mi)
{
	SceneTree *tree = reinterpret_cast<SceneTree *>(Engine::get_singleton()->get_main_loop());
	int64_t window_id = tree->get_root()->get_window()->get_window_id();

	Ref<InputEventMouseButton> mb;
	mb.instantiate();
	mb->set_window_id(window_id);
	mb->set_device(mi->id);
	if (rm.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
	{
		mi->leftButtonDown = true;
		mb->set_button_index(MOUSE_BUTTON_LEFT);
		mb->set_pressed(true);
	}
	if (rm.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
	{
		mi->leftButtonDown = false;
		mb->set_button_index(MOUSE_BUTTON_LEFT);
		mb->set_pressed(false);
	}
	if (rm.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
	{
		mi->rightButtonDown = true;
		mb->set_button_index(MOUSE_BUTTON_RIGHT);
		mb->set_pressed(true);
	}
	if (rm.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
	{
		mi->rightButtonDown = false;
		mb->set_button_index(MOUSE_BUTTON_RIGHT);
		mb->set_pressed(false);
	}
	if (rm.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
	{
		mi->middleButtonDown = true;
		mb->set_button_index(MOUSE_BUTTON_MIDDLE);
		mb->set_pressed(true);
	}
	if (rm.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
	{
		mi->middleButtonDown = false;
		mb->set_button_index(MOUSE_BUTTON_MIDDLE);
		mb->set_pressed(false);
	}
	mb->set_button_mask(
		MOUSE_BUTTON_MASK_LEFT * mi->leftButtonDown |
		MOUSE_BUTTON_MASK_RIGHT * mi->rightButtonDown |
		MOUSE_BUTTON_MASK_MIDDLE * mi->middleButtonDown);

	// handle mouse wheel
	if (rm.usButtonFlags & RI_MOUSE_WHEEL)
	{
		if (rm.usButtonData > 0)
		{
			mb->set_button_index(MOUSE_BUTTON_WHEEL_UP);
			mb->set_pressed(true);
		}
		else if (rm.usButtonData < 0)
		{
			mb->set_button_index(MOUSE_BUTTON_WHEEL_DOWN);
			mb->set_pressed(true);
		}
		mb->set_factor(abs(rm.usButtonData) / WHEEL_DELTA);
	}

	RawInput::get_singleton()->emit_signal("input_event", mb);

	// trigger mouse wheel release
	if (mb->get_button_index() == MOUSE_BUTTON_WHEEL_UP || mb->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)
	{
		Ref<InputEventMouseButton> mbr = mb->duplicate();
		mbr->set_pressed(false);
		RawInput::get_singleton()->emit_signal("input_event", mbr);
	}

	// handle mouse movement
	if (rm.lLastX != 0 || rm.lLastY != 0)
	{
		Vector2 motion = Vector2(rm.lLastX, rm.lLastY);
		mi->position += motion;

		Ref<InputEventMouseMotion> mm;
		mm.instantiate();
		mm->set_window_id(window_id);
		mm->set_relative(motion);
		mm->set_device(mi->id);
		mm->set_pressure(mi->leftButtonDown ? 1.0f : 0.0f);
		mm->set_position(mi->position);
		mm->set_global_position(mi->position);
		RawInput::get_singleton()->emit_signal("input_event", mm);
	}
}

void handle_keyboard(RAWKEYBOARD rk, int id)
{
	// handle keyboard input
	if (rk.Flags & RI_KEY_MAKE)
	{
		RawInput::get_singleton()->emit_signal("key_pressed", rk.VKey, id);
	}
	if (rk.Flags & RI_KEY_BREAK)
	{
		RawInput::get_singleton()->emit_signal("key_released", rk.VKey, id);
	}
}

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Call the original window procedure first
	LRESULT retval = CallWindowProc(g_oldWndProc, hwnd, uMsg, wParam, lParam);

	// Tack on our raw input logic on the end
	if (uMsg == WM_INPUT)
	{
		UINT dataSize;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
		if (dataSize > 0)
		{
			RAWINPUT *rawInput = (RAWINPUT *)malloc(dataSize);
			if (rawInput)
			{
				if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInput, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
				{
					if (rawInput->header.dwType == RIM_TYPEMOUSE)
					{
						// handle mouse input
						RAWMOUSE rm = rawInput->data.mouse;
						HANDLE device = rawInput->header.hDevice;
						if (!mouseInfos.has(device))
						{
							MouseInfo *mi = new MouseInfo();
							mi->id = mouseInfos.size();
							mouseInfos.insert(device, mi);
						}
						int id = mouseInfos[device]->id;

						handle_mouse(rm, mouseInfos[device]);
					}
					else if (rawInput->header.dwType == RIM_TYPEKEYBOARD)
					{
						// handle keyboard input
						RAWKEYBOARD rk = rawInput->data.keyboard;
						HANDLE device = rawInput->header.hDevice;
						if (!keyboardIds.has(device))
						{
							keyboardIds.insert(device, keyboardIds.size());
						}
						int id = keyboardIds[device];
						handle_keyboard(rk, id);
					}
				}
				free(rawInput);
			}
		}
	}

	// return the original window procedure's return value
	return retval;
}