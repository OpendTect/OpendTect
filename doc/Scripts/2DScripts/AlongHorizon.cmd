dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "-------Volume Between two horizons------"
Case Insensitive

Menu "Processing`Create Volume output`Along horizon"
Window "Create Hori*"
Button "Select Quantity to output"
ListClick "Select Data" "LS 5k" Double
Button "Select Calculate along*"
ListClick "Objects list" "horizon1" Double
Button "Select Trace subsel*"
Window "Positions"
Input "Z start" 0
Input "Z stop" 5000
Ok
Button "One line only" On
Input "Z Interval Start" 10
Input "Z Interval Stop" 20
Button "Select Store in*"
Input "Name" "Test"
Input "Attribute" "Along-Horizon"
Ok
Button "Proceed"
#Later change this option to Output window
Window "Batch launch"
Combo "Output to" "Standard output"
Ok
Sleep 6

TreeMenu "2D Seismics" "Add"
ListClick "Objects list" "Test" Double
ListClick "Select Data*" 1 Double
TreeMenu "2D Seismics`*`*`*" "Select*`Stored 2D Data`Along-Horizon"
Snapshot "$SNAPSHOTSDIR$/$IDX$_Along-Horizon.png" ODMain
Sleep 2

Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Test"
Button "Remove this*"
Button "Yes"
Button "Dismiss"
Menu "Survey`Select/Setup"
Ok

End


