dTect V3.2
OpendTect commands
Mon Jun  11:36:39 2008
!

#Create ChronoStratigraphy Creation
Comment "---------ChronoStratigraphy Creation-----------"

Case Insensitive
Menu "SSIS`Chrono Stratigraphy`New"
Window "ChronoStrati*"
Button "Select LineSet*"
Button "Select Input Line Set"
ListClick "Objects list" "LS 5k" Double
ListButton "Line names" "i5007" On
Ok

Button "Read horizons"
ListSelect "Objects list" 1 2 On
Ok

Comment "---------Table Selection----------"

TableExec "Sequence Table" 1 2 Combo "Model 0" "Event*"
TableExec "Sequence Table" 2 2 Button "Settings 0"
Button "Select Input Line Set"
ListClick "Objects list" "LS 5k"  Double
Sleep 1
Ok
Button "Select Output ChronoStratigraphy"
Input "Name" "test_2D_ChronoStrat"
Ok
Button "Proceed"
Ok
Sleep 15

Comment "-----------Displaying ChronoStratigraphy-----------"

Wheel "hRotate" 45
Slider "Zoom Slider" 25

Menu "SSIS`Chrono Stratigraphy`Select"
ListClick "Objects list" "test_2D_ChronoStrat" Double
Menu "SSIS`Add Wheeler Scene"

TreeMenu "Tree scene 1" "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree scene 1" "2D Seismics`*`*`*" "Select Attr*`Stored 2D Data`Seis"
TreeMenu "Tree scene 1" "2D Seismics`*`*" "Add chronostratigraphy*"

TreeMenu "Tree scene 2" "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree scene 2" "2D Seismics`*`*`*" "Select Attr*`Stored 2D Data`Seis"

Snapshot "$SNAPSHOTSDIR$/$IDX$_2D-ChronoStrat.png" ODMain
Wheel "hRotate" -55
Slider "Zoom Slider" 15

Menu "Survey`Manage`Chrono Stratigraphy"
ListClick "Objects list" "test_2D_ChronoStrat"
Button "Remove this Object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Select/Setup"
Ok

End

