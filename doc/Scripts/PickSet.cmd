dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

TreeMenu "PickSet" "Load"
ListClick "Objects list" 1
Ok

#Applying Attribute on tree item PickSet

Comment "------------Applying Attribute on tree item PickSet----------"
TreeMenu "PickSet" "New`Empty*"
Input "Name*" "newPickset"
Button "Color"
ColorOk Red 2
Ok
TreeMenu "PickSet" "Remove all items"
Button "Yes"

End
