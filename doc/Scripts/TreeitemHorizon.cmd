dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Comment "----------Applying all Horizon options on an Attribute---------"

TreeMenu "Horizon" "Load"
ListClick "Select Horizon*" 1 Double
Wheel "vRotate" 45
TreeMenu "Horizon`*" "Add attribute"
TreeMenu "Horizon`*`<right-click>" "Select Att*`Stored*`Median Dip*"
TreeMenu "Horizon`*`Median*" "Show Histogram"
Window "Data statistics"
Snapshot "$SNAPSHOTSDIR$/Hor-histogram.png"
Sleep 2
Close
TreeMenu "Horizon`*`Median*" "Show Amplitude*"
Window "Amplitude Spectrum"
Snapshot "$SNAPSHOTSDIR$/Hor-Amplitudespectrum.png"
Sleep 2
Close
TreeMenu "Horizon`*" "Tracking`Wireframe"
Sleep 2

TreeMenu "Horizon`*" "Shift"
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

Menu "Survey`Select/Setup"
Button "No"
Ok

End
