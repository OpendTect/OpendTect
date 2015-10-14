dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "------------2D-RGB Canvas (Color Table Manager)----------"
Case Insensitive

TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "2D Seismics`*`*`*" "Select Attribute`Stored 2D data`Seis"
Wheel "hRotate" -50
CanvasMenu "RGB Array view" "Manage"
Button "Segmentize" On
Input "Segmentize" 6
Button "Undefined color"
ColorOK Red 2
Button "Save as"
Input "Name" "2D-Color-edit"
Ok

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_2d-ColorTabManager"
Ok
Wheel "hRotate" 50

CanvasMenu "RGB Array view" "Manage"
Button "Remove"
Button "Yes"
Ok
TreeMenu "2D Seismics`*" "Remove"
Button "Yes"

End

