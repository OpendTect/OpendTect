dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Comment "----------Creating 2D-Horizon Grid-----------"

Case Insensitive
TreeMenu "2D Horizon" "Load"
ListClick "Objects list" 1 Double

TreeMenu "2D Horizon`*" "Derive 3D horizon"
Input "Search radius" 150
Button "Select Output Horizon" 
Input "Name" "TestHorizon"
Ok
Ok

Menu "Processing`Create Grid output`Grid"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Input "Attribute name" "LS5k-2dHorGrid"
Button "Select Calculate*"
ListClick "Objects list" "TestHorizon" Double
Button "Proceed"
Ok
Sleep 65

Window "OpendTect*"
TreeMenu "Horizon" "Load"
ListClick "Objects list" "TestHorizon" Double
TreeMenu "Horizon`*`Z values" "Select Attribute`Surface data*"
ListClick "Select Data*" "LS5k-2dHorGrid" Double

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_2d-horizon-grid"
Ok

TreeMenu "Horizon`*" "Remove"
TreeMenu "2D Horizon`*" "Remove"
Button  "Manage horizons"
ListClick "Objects list" "TestHorizon"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

End

