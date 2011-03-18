dTect V3.2
OpendTect commands
Mon Jun 7 11:36:39 2008
!

Case Insensitive
Wheel "vRotate" 85
Comment "---------Filling Holes in a Horizon---------------"

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double
TreeMenu "Horizon`*" "Algorithms`Snap to event"
Button "Select Input Cube"
ListClick "Objects list" "Median Dip Filtered Seis*" Double
Combo "Event" 1
Input "Search gate (ms) start" -10
Input "Search gate (ms) stop" 10
Button "As new"
Button "Select Output Horizon"
Input "Name" "FS4-Snap-to-event"
Ok
Ok
#snapshot
Button "Make snapshot"
Button "Window"
Ok
Input "Filename" "$SNAPSHOTSDIR$/$FILEIDX$_FS4-Snap-to-event.png"
Ok
Sleep 3
Wheel "vRotate" -85

TreeMenu "Horizon`*" "Save as"
Button "Select Output Surface"
Input "Name" "FS4-with Holes"
Ok
Ok

TreeMenu "Horizon`*" "Algorithms`Fill holes"
Ok
Ok

TreeMenu "Horizon`*" "Save as"
Button "Select Output Surface"
Input "Name" "FS4-without Holes"
Ok
Ok
TreeMenu "Horizon`*" "Remove"
#Button "No"
#TreeMenu "Horizon`FS4-Snap-to-event" "Remove"
#Button "No"

Comment "---------Diaplaying Horizon without Holes------------"
Menu "Scenes`New"
Menu "Scenes`Tile`Horizontal"
Wheel "vRotate" 85

TreeMenu "Tree scene 1" "Horizon" "Load"
ListClick "Objects list" "FS4-with Holes" Double

TreeMenu "Tree scene 2" "Horizon" "Load"
ListClick "Objects list" "FS4-without Holes" Double

Button "Make snapshot"
Button "Window"
Ok
Input "Filename" "$SNAPSHOTSDIR$/$FILEIDX$_horizon-fill-holes.png"
Ok
Wheel "vRotate" -85

Menu "Survey`Manage`Horizons"

ListClick "Objects list" "FS4-with Holes" 
Button "Remove this object"
Button "Yes"

ListClick "Objects list" "FS4-without Holes"
Button "Remove this object"
Button "Yes"

ListClick "Objects list" "FS4-Snap-to-event"
Button "Remove this object"
Button "Yes"

Button "Dismiss"

Menu "Survey`Select/Setup"
Ok

End
