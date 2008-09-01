dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "--------------Create Seismic output---------------"
Case Insensitive
Menu "Processing`Create Seismic output"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Button "Select Trace subsel*"
Input "Number start" 1100
Input "Number stop" 5000
Input "Z Start" 0
Input "Z Stop" 5000
Ok
Button "One line only" On
Combo "One line only" "i5007"

Button "Select Store in Line Set"
Input "Name" "Seis-output"
Ok
Button "Proceed"
Ok
Sleep 5
Comment "----------Displaying created Seismic output----------"
TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "Seis-output" Double
ListClick "Select Data from List" 1 Double
TreeMenu "2D Seismics`*`*`*" "Select*`Stored 2D Data`Seis"

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/2d-ceateSeismic-output"
Ok

TreeMenu "2D Seismics`*" "Remove"
Button "Yes"

End
