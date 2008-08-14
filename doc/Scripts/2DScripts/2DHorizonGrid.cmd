dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Comment "----------Creating 2D-Horizon Grid-----------"

Case Insensitive
TreeMenu "2D Horizon" "Load"
ListClick "Select 2D Hori*" 1 Double

TreeMenu "2D Horizon`*" "Derive 3D horizon"
Input "Search radius" 150
Button "Select Output Horizon" 
Input "Name" "TestHorizon"
Ok
Ok

Menu "Processing`Create output using Horizon`Horizon grid"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Input "Attribute name" "LS5k-2dHorGrid"
Button "Select Calculate*"
ListClick "Objects list" 1 Double
Button "Proceed"
Ok
Sleep 65

Window "OpendTect*"
TreeMenu "Horizon" "Load"
ListClick "Select Horizon*" 1 Double
TreeMenu "Horizon`*`Z values" "Select Attribute`Surface data*"
OnError Continue
ListClick "Select Data*" "LS5k-2dHorGrid" Double
Ok
OnError Stop

End
