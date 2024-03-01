extends Node2D

var cursors : Array[Polygon2D] = []

func _ready() -> void:
	# For convenience sake. Raw mouse input only works when the window is focused,
	# So we want to keep the real mouse in the window to stop it from clicking elsewhere
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

	for i in RawInput.get_mouse_device_count():
		var cursor = $Cursor.duplicate() as Polygon2D
		cursor.color = Color.from_hsv(i * 0.2, 0.9, 1.0)
		cursor.position += Vector2(64, 0).rotated(i * 0.2)
		RawInput.warp_mouse(cursor.position, i)
		add_child(cursor)
		cursors.append(cursor)

	RawInput.input_event.connect(_on_raw_input_input_event)

func _process(delta: float) -> void:
	for i in cursors.size():
		var cursor = cursors[i]
		cursor.position = RawInput.get_mouse_position(i)
		if RawInput.is_mouse_button_pressed(MOUSE_BUTTON_LEFT, i):
			$Cursor.position = $Cursor.position.move_toward(cursor.position, 300*delta)

# recenter on middle mouse button
func _on_raw_input_input_event(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.pressed and event.button_index == MOUSE_BUTTON_MIDDLE:
			RawInput.warp_mouse(get_viewport_rect().size/2, event.device)
