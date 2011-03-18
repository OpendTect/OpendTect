dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Comment "------------Scene Properties-----------------"

Case Insensitive

TreeMenu "Inline" "Add"
TreeMenu "Inline`*`*" "Select Attribute`Stored*`Median Dip*"

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double
TreeMenu "Horizon`*" "Add attribute"
TreeMenu "Horizon`*`<right-click>" "Select Att*`Stored*`Median Dip*"

TreeMenu "Horizon`*" "Create flattened scene"
TreeMenu "Tree Scene 2" "Inline" "Add"
TreeMenu "Tree Scene 2" "Inline`*`<right-click>" "Select Att*`Stored*`Median Dip*"

TreeMenu "Scene 1" "Properties"
Button "Survey box" Off
Sleep 2
Button "Survey box" On
Sleep 1
Button "Annotation text" Off
Sleep 2
Button "Annotation text" On
Sleep 1
Button "Annotation Scale" Off
Sleep 2
Button "Annotation Scale" On
Sleep 1
Button "Background color"
ColorOK Cyan 4
Slider "Marker size Slider" 39
Button "Mouse marker color"
ColorOK Black 2
Button "Apply to all scenes" On
Ok

Button "Make Snapshot"
Button "Window"
Ok
Input "Filename" "$SNAPSHOTSDIR$/$FILEIDX$_Flatten_scene.png"
Ok

Menu "Survey`Select/Setup"
Ok
End
