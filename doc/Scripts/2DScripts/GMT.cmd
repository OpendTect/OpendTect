dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive
Comment "-----------Preparing GMT-----------"

Menu "Processing`GMT Mapping Tool"
Window "GMT Mapping*"
Tab "Tab" "Contour"
Button "Select Select Horizon"
ListClick "Objects list" 1 Double
Button "Read Horizon"
Sleep 4
Button "Add"
Tab "Tab" "Locations"
Button "Select Select Pickset"
ListClick "Objects list" 1 Double
Sleep 2
Button "Add"
Input "Select output*" "$SNAPSHOTSDIR$/$IDX$_2d-map" 
Button "Create Map"

#----Change this option later---
Window "Batch launch"
Combo "Output to" "Standard*"
Ok
#---------------------------

Sleep 200
Button "View Map"
Window "GMT Mapping*"
Button "Dismiss"

Menu "Survey`Select/Setup"
Ok

End



