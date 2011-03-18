dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive
TreeMenu "PickSet" "Load"
ListClick "Objects list" 1
Ok

Comment "------------managing PickSet----------"
TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 260
Ok
ListClick "Select Data" "Median Dip*" Double

TreeMenu "PickSet" "New`Empty*"
Input "Name*" "newPicksetCreate"
Button "Color"
ColorOk Red 2
Ok
TreeMenu "PickSet" "Remove all items"
Button "Remove"
TreeMenu "PickSet" "Load"
ListClick "Objects list" 1 double
TreeMenu "PickSet`*" "Save As"
Input "Name" "TestPickSet"
Ok
TreeMenu "PickSet`*" "Display only at sections"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_PickSetatSections.png" CurWin
TreeMenu "PickSet`*" "Display only at sections"
TreeMenu "PickSet`*" "Set directions"
Window "Add direction*"
Button "Steering cube"
Button "Select Steering Data" 
ListClick "Objects list" "Steering Cube BG Detailled" Double
Ok
TreeMenu "PickSet`*" "Properties"
Window "Pick properties"
Combo "Shape" "Cube"
Combo "Shape" "Cone"
Combo "Shape" "Cylinder"
Combo "Shape" "Cross"
Combo "Shape" "Arrow"
Button "Connect picks" On
Button "Surface"
Ok
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_ConnectPicks_Surface.png" CurWin
TreeMenu "PickSet`*" "Properties"
Button "Connect picks" On
Button "Line"
Input "Size value" 5
Ok
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_ConnectPicks_Line.png" CurWin
TreeMenu "PickSet`*" "Properties"
Button "Connect picks" Off
Ok
TreeMenu "PickSet`*" "Save"
TreeMenu "PickSet`*" "Remove"

Button "Manage Pick Sets"
ListClick "Objects list" "TestPickSet"
Button "Rename this object"
Input "New name" "TestPickSet-new"
Button "Merge pick sets"
ListClick "Objects list" 2 
ListSelect "Objects list" "TestPickSet-new" On
Button "Select Output*"
Input "Name" "MergedPickSets"
Ok
Ok

ListClick "Objects list" "MergedPickSets"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "TestPickSet-new"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "newPicksetCreate"
Button "Remove this object"
Button "Yes"
Button "Dismiss"
TreeMenu "Inline`*" "Remove"

End
