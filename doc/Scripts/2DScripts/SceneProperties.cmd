dTect V3.2
OpendTect commands
Mon Jun 7 11:36:39 2008
!

Case Insensitive
Comment "---------Scene Properties-----------"

Menu "Scenes`New"
Menu "Scenes`Tile`Horizontal"

TreeMenu "Scene 1" "Properties"
Button "Survey box" Off
Sleep 3
Button "Survey box" On
Button "Annotation text" Off
Sleep 3
Button "Annotation text" On
Button "Annotation scale" Off
Sleep 3
Button "Annotation scale" On 
Button "Background color"
ColorOK Cyan 4
Sleep 2
Slider "Marker size Slider" 39
Button "Mouse marker color"
ColorOK Black 2
Button "Apply to all scenes" On
Ok

Snapshot "$SNAPSHOTSDIR$/$IDX$_2d-sceneproperties.png" ODMain

Menu "Survey`Select/Setup"
Ok

End

