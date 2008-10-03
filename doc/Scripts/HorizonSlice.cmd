dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Creating Output Using HorizonSlice---------"
Case Insensitive

Menu "processing`Create output using Horizon`Horizon slice"
Button "Select Quantity*"
Button "Stored"
ListClick "Select Data" "Median Dip Filtered*" Double
Button "Select Calculate along surface"
ListClick "Objects list" 1 Double
Button "Select Volume*"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Input "Z Interval Start" -100
Input "Z Interval Stop" 100
Button "Select Output Cube"
Input "Name" "Mediandip_HorSlice_Demo0"
Ok
Button "Proceed"
Ok
Sleep 10

TreeMenu "Inline" "Add"
TreeMenu "Inline`425" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok

Button "Stored"
ListClick "Select Data" "Mediandip_HorSlice_Demo0" Double

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/Mediandip_HorSlice.jpg"
Ok

TreeMenu "Inline`*" "Remove"
Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Mediandip_HorSlice_Demo0"
Button "Remove this Object"
Sleep 2
Button "Yes"
Button "Dismiss"

#"Parameter id is not correct" error
# is coming After running this script second time
# to rectify this problem select same survey again

Menu "Survey`Select/Setup"
Sleep 2
Ok

End
