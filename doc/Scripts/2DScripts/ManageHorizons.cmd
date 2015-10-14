dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive
Comment "-----------Manageing Horizons------------"

Menu "Survey`Manage`Horizons 2D"
Window "Surface file*"
ListClick "Objects list" "horizon1"
Button "Copy 2D*"
Button "Select Output*"
Input "Name" "horizon1-copy"
Ok
Ok
ListClick "Objects list" "horizon2"
Button "Copy 2D*"
Button "Select Output*"
Input "Name" "horizon2-copy"
Ok
Ok
Button "Relations"
Button "Read Horizons"
ListClick "Objects list" "horizon1-copy" On
ListSelect "Objects list" "horizon2-copy" On
Ok
Button "Check crossings"
Button "Select Horizon"
Input "Name" "horizon-crossings"
Ok
Ok
Button "Dismiss"
Window "Surface file*"
ListClick "Objects list" "horizon1-copy"
Button "Rename this*"
Input "New name" "horizon1-rename"
ListClick "Objects list" "horizon2-copy"
Button "Rename this*"
Input "New name" "horizon2-rename"
ListClick "Objects list" "horizon1-rename"
Button "Remove this*"
Button "Yes"
ListClick "Objects list" "horizon2-rename"
Button "Remove this*"
Button "Yes"
ListClick "Objects list" "horizon-crossings"
Button "Remove this*"
Button "Yes"

Button "Dismiss"
End

