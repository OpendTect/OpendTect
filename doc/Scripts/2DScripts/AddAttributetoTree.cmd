dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Adding One StoredAttribute to All Tree items----------"

Case Insensitive

TreeMenu "Scene 1" "Properties"
Button "Background color"
ColorOK Cyan 4
Ok

Comment "------Adding Items to tree menu----------------"

Wheel "hRotate" -45
Slider "Zoom slider" 20

TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" double
ListClick "Select Data from List" "i5007" Double
TreeMenu "2D Seismics`*`*`*" "Select Attribute`Stored 2D data`Seis"
TreeClick "2D Seismics`*`*`Seis"
Combo "Table selection" "Altimetric" Double

TreeMenu "PickSet" "Load"
ListClick "Objects list" "demo*" Double

TreeMenu "2D Horizon" "Load"
ListClick "Objects list" 1 Double

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double

Wheel "hRotate" 45
Sleep 5
Slider "Zoom slider" 29
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_2d_TreeItems"
Ok

Menu "Survey`Select/Setup"
Ok

End

