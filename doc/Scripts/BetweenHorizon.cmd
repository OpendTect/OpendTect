dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Creating Output Using BetweenHorizons-------------"

#Menu "Survey`Select/Setup"
#Ok

Case Insensitive
Menu "processing`Create output using Horizon`Between horizons"
Button "Select Quantity*"
Button "Stored"
ListClick "Select Data" "Median Dip Filtered*" Double
Button "Select Calculate between*"
ListClick "Objects list" 1  Double
Button "Select and bottom surface*"
ListClick "Objects list" 2  Double
Button "Select Volume*"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Button "Select Output Cube"
Input "Name" "Test-zone"
Ok

Button "Proceed"
Ok
Sleep 8

TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok

Button "Stored"
ListClick "Select Data" "Test-zone" Double
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename"  "$SNAPSHOTSDIR$/betHor-testzone_1.jpg" 
Ok

TreeMenu "Inline`*" "Remove"
Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Test-zone"
Button "Remove this object"
Sleep 3
Button "Yes"
Button "Dismiss"

Menu "Survey`Select/Setup"
Sleep 3
Ok

End
