dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive
TreeMenu "PickSet" "Load"
ListClick "Objects list" 1
Ok

#Applying Attribute on tree item PickSet

Comment "------------Applying Attribute on tree item PickSet----------"
TreeMenu "PickSet" "New`Empty*"
Input "Name*" "newPicksetCreate"
Button "Color"
ColorOk Red 2
Ok
TreeMenu "PickSet" "Remove all items"
Button "Yes"

Button "Manage Pick Sets"
ListClick "Objects list" "newPicksetCreate"
Button "Remove this Object"
Button "Yes"
Button "Dismiss"

End
