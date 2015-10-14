dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Importing Attribute Set--------"

Menu "Survey`Select/Setup"
ListClick "Select Data" "Magellane*" Double
Include "$SCRIPTSDIR$/2DScripts/AllAtributes.cmd"
Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "Save attribute set"
Input "Name" "DemoAttributeSet"
Ok
Ok
Menu "Survey`Select/Setup"
ListClick "Select Data" "F3_Demo_for_Test" Double
Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "Import attribute set*"
ListClick "Select Data*" "Magellane*" Double
ListClick "Objects list" "DemoAttributeSet" Double
Button "Select Input Seismics*"
ListClick "Objects list" "Median Dip Fil*" Double
Button "Select Input Steering cube"
ListClick "Objects list" "Steering Cube BG Detailled" Double
Ok
Button "Move Down"
Button "Move Down"
Button "Move Down"
ListClick "Defined Attributes" "PositionAttrib"
Button "Move Up"
Button "Move Up"
Button "Move Up"
Button "Sort"
Ok
Menu "Survey`Select/Setup"
Ok
Button "No"

Menu "Survey`Select/Setup"
ListClick "Select Data" "Magellane*" Double
Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "Open attribute set"
ListClick "Objects list" "DemoAttributeSet"
Button "Rename this*"
Input "New name" "DemoAttributeSet-new"
ListClick "Objects list" "DemoAttributeSet-new"
Button "Remove this*"
Button "Yes"
Button "Cancel"
Button "Cancel"
Menu "Survey`Select/Setup"
ListClick "Select Data" "F3_Demo_for_Test" Double

End

