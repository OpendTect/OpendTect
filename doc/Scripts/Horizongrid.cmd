dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Creating Output using HorizonGrid-------------"

Case Insensitive
Wheel "vRotate" 45
Menu "processing`Create Grid output`Grid"
Button "Select Quantity*"
Button "Stored"
ListClick "Select Data" "Median Dip Filtered Seismics" Double
Button "Select Calculate on surface"
ListClick "Objects list" 1  Double
Input "Attribute name" "MedianDipFiltSeis-FS4"
Button "Proceed"
Ok
Sleep 180 

Window "OpendTect*"
TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double
TreeMenu "Horizon`Demo 0*`Z values" "Select Attribute`Surface data*"
ListClick "Select Data from List" "MedianDipFiltSeis-FS4" Double
Button "Make Snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_Horizongrid-mediandip-FS4"
Ok

Wheel "vRotate" -45

TreeMenu "Horizon`*" "Remove"

Button "manage horizons"
Window "Surface file*"
ListClick "Objects list" 1
ListClick "Calculated attributes" "MedianDipFiltSeis-FS4"
Button "Remove selected*"
Button "Yes"
Button "Dismiss"

End
