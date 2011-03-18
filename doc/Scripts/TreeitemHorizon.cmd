dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Comment "----------Applying all Horizon options on an Attribute---------"

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double
Wheel "vRotate" 45
TreeMenu "Horizon`*" "Add attribute"
TreeMenu "Horizon`*`<right-click>" "Select Att*`Stored*`Median Dip*"
TreeMenu "Horizon`*`Median*" "Show Histogram"
Window "Data statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Hor-histogram.png"
Sleep 2
Close
TreeMenu "Horizon`*`Median*" "Show Amplitude*"
Window "Amplitude Spectrum"
Snapshot "$SNAPSHOTSDIR$/$Hor-Amplitudespectrum.png"
Sleep 2
Close
TreeMenu "Horizon`*" "Tracking`Wireframe"
Sleep 2

TreeMenu "Horizon`*" "Shift"
Window "Horizon shift"
Slider "Horizon slider" 0
sleep 2
Slider "Horizon slider" 100
sleep 2
Slider "Horizon slider" 50
sleep 2
Ok

Wheel "vRotate" -45

TreeMenu "Inline" "Add"
TreeMenu "Inline`*`<right-click>" "Select Att*`Stored*`Median Dip*"
TreeMenu "Horizon`*" "Display only*"

TreeMenu "Horizon`*" "Create flattened scene"
TreeMenu "Tree scene 2" "Inline" "Add"
TreeMenu "Tree scene 2" "Inline`*`<right-click>" "Select Att*`Stored*`Median Dip*"

Comment "-----------------Managing horizons-----------------"

Menu "Survey`Manage`Horizons"
ListClick "Objects list" 1
Button "Copy 3D Horizon"
Button "Select Output Horizon"
Input "Name" "FS4-copy"
Ok
Ok
ListClick "Objects list" "FS4-copy"
Button "Rename this object"
Input "New name" "FS4-New"
ListClick "Objects list" "FS4-New"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Select/Setup"
ListClick "Select Data" "F3_Demo_for_Test" Double

End
