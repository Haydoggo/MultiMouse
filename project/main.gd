extends Node2D

@onready var markers : Array[Node2D] = [$Cursor1, $Cursor2, $Cursor3]

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_CONFINED)
	RawInput.input_event.connect(_on_raw_input_input_event)

func _process(delta: float) -> void:
	for i in 2:
		markers[i].position = RawInput.get_mouse_position(i)
		if RawInput.is_mouse_button_pressed(MOUSE_BUTTON_LEFT, i):
			markers[2].position = markers[2].position.move_toward(markers[i].position, 300*delta)

func _on_raw_input_input_event(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.pressed and event.button_index == MOUSE_BUTTON_MIDDLE:
			RawInput.warp_mouse_position(get_viewport_rect().size/2, event.device)
