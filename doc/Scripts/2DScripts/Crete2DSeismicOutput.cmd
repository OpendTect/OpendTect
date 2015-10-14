dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "--------------Create Seismic output---------------"
Case Insensitive
Menu "Processing`Create Volume output`Cube"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Button "Select Trace subsel*"
Input "Number start" 238
Input "Number stop" 600
Input "Z start" 0
Input "Z stop" 5000
Ok
Button "One line only" On
Combo "One line only" "i5007"
Button "Select Store in Line Set"
Input "Name" "Seis-output-vol-1"
Input "Attribute" "Seis"
Ok

#todo later change this option to Output ot window---------
Button "Proceed"
Button "Yes"
Window "Batch launch"
Combo "Output to" "Standard output"
Ok
Sleep 5

Menu "Processing`Create Volume output`Cube"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Button "Select Trace subsel*"
Input "Number start" 601
Input "Number stop" 1200
Input "Z start" 0
Input "Z stop" 5000
Ok
Button "One line only" On
Combo "One line only" "i5007"
Button "Select Store in Line Set"
Input "Name" "Seis-output-vol-2"
Input "Attribute" "Seis"
Ok

#todo later change this option to Output ot window---------
Button "Proceed"
Button "Yes"
Window "Batch launch"
Combo "Output to" "Standard output"
Ok
Sleep 5

Comment "----------Displaying created Seismic output----------"
TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "Seis-output-vol-1" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "2D Seismics`*`*`*" "Select*`Stored 2D Data`Seis"

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_2d-ceateSeismic-output"
Ok

TreeMenu "2D Seismics`*" "Remove"
Button "Yes"

Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Seis-output-vol-1"
Button "Copy cube"
ListClick "Attributes" "Seis"
Button "Browse/edit*"
Window "Browse seismic*"
Button "Goto position"
Input "Inline" 250
Ok
Button "Information"
Window "Selected Trace*"
Button "Dismiss"
Button "Move left"
Button "Move right"
Button "Show Wiggle"
Window "Browse*"
Button "Cancel"
ListClick "Attributes" "Seis"
Button "Rename Attribute"
Input "New name" "Seis-copy"
#Ok
Button "Dismiss"

Window "Seismic file*"
ListClick "Objects list" "Seis-output-vol-1"
Button "Rename*"
Input "New name" "Seis-vol-1"

ListClick "Objects list" "Seis-vol-1"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "Seis-output-vol-2"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

End

