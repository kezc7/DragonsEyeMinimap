# MiniMap

## Settings

`Data/SKSE/Plugins/DragonsEyeMinimap.ini`

```ini
[Controls]
; Keyboard key to toggle/control the minimap, as a DirectInput scan code (e.g. 38 = L, 50 = M).
; 0 uses the key bound to "Local Map" in the game's control map.
uToggleKey=0

; Gamepad button to toggle/control the minimap, 0 = the game's "Wait" control.
; 1 = DPad Up, 2 = DPad Down, 4 = DPad Left, 8 = DPad Right, 16 = Start, 32 = Back, 64 = LS, 128 = RS,
; 256 = LB, 512 = RB, 4096 = A, 8192 = B, 16384 = X, 32768 = Y
uGamepadToggleKey=0

; Gamepad button to hold together with uGamepadToggleKey, 0 = none (e.g. 512 + 32 = RB + Back)
uGamepadToggleModifier=0
```
