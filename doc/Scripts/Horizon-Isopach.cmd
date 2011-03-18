dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "------Calculating Thickness Between 2 selected Horizons--------"

Case Insensitive

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double
Wheel "vRotate" 45
TreeMenu "Horizon`*" "Calculate Isopach"
Button "Select Calculate to"
ListClick "Objects list" 4 double
Input "Attribute name" "Demo-Isopach"
Ok

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_Demo-Isopach.jpg"
Ok
Wheel "vRotate" -45

TreeMenu "Horizon`*" "Remove"

End
