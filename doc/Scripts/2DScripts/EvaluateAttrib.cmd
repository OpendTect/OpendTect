dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive
Comment "-------Evaluete Energy Attribute----"

TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data*" "i5007" Double
TreeClick "2D Seismics`*`*`*"
Menu "Analysis`Attributes"
Window "Attribute Set*"
Combo "Attribute type" "Energy"
Button "Select Input Data"
ListClick "Select Data" "LS 5k" Double
Input "Attribute name" "EnetgyAttrib"
Button "Save on OK" Off
Button "Evaluate Attri*"
Window "Evaluate*"
Input "Nr of slices" " 3
Button "Calculate"
Sleep 2
Slider "Slice Slider" 0
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_evaluate_energy_28-28"
Ok
Sleep 2
Window "Evaluate attribute"
Slider "Slice Slider" 50
Sleep 1
 
Slider "Slice Slider" 100
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_evaluate_energy_36-36"
Ok
Window "Evaluate attribute"
Button "Accept"
Window "Attribute Set*"
Button "Cancel"
TreeMenu "2D Seismics`*" "Remove"
Button "Yes"

End

