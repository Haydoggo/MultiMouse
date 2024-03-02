#ifndef RAW_INPUT_H
#define RAW_INPUT_H

#include <Windows.h>
#include <WinUser.h>
#include <godot_cpp/classes/object.hpp>


LRESULT CALLBACK MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace godot {

class RawInput : public Object {
	GDCLASS(RawInput, Object)
	static RawInput *singleton;
private:
	static void initialize();
protected:
	static void _bind_methods();

public:
	_FORCE_INLINE_ static RawInput *get_singleton() {
		if (unlikely(singleton == nullptr)){
			new RawInput();
		}
		return singleton;
	}

	RawInput();
	~RawInput();
	void warp_mouse(Vector2 position, int id);
	bool is_mouse_button_pressed(MouseButton button, int id);
	Vector2 get_mouse_position(int id);
	int get_mouse_device_count();
};

struct MouseInfo {
	Vector2 position = Vector2(0,0);
	int id = 0;
	bool leftButtonDown = false;
	bool rightButtonDown = false;
	bool middleButtonDown = false;
};


}

#endif