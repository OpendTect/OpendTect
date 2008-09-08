dTect V3.2
OpendTect commands
Mon Jun 7 11:36:39 2008
!

Case Insensitive
Comment "---------Filling Holes in a Horizon---------------"

TreeMenu "Horizon" "Load"
ListClick "Select Horizon*" "Horizon with holes" Double
TreeMenu "Horizon`*" "Algorithms`Fill holes"
Ok
Ok
TreeMenu "Horizon`*" "Save as"
Button "Select Output Surface"
Input "Name" "Horizon without holes"
Ok
Ok
TreeMenu "Horizon`*" "Remove"
Button "No"

Comment "---------Diaplaying Horizon without Holes------------"
Menu "Windows`New"
Menu "Windows`Tile`Horizontal"
Wheel "vRotate" 85

TreeMenu "Tree scene 1" "Horizon" "Load"
ListClick "Select Horizon*" "Horizon with holes" Double

TreeMenu "Tree scene 2" "Horizon" "Load"
ListClick "Select Horizon*" "Horizon without holes" Double

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/horizon-fill-holes"
Ok
Wheel "vRotate" -85

Menu "Survey`Manage`Horizons"
ListClick "Objects list" "Horizon without holes" 
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Select/Setup"
Ok
