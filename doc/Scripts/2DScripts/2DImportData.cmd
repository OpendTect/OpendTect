dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Importing Pickset--------------"

Menu "Survey`Import`Picksets"
Input "Input Ascii file" "$EXPORTDIR$/Pickset2D-Export.asc" 
Button "Define Format def*"
Ok
Button "Select Output Pickset"
Input "Name" "Pickset2D-Import"
Ok
Button "Go"
Ok

Sleep 2
Button "Dismiss"

Comment "----------Importing 2D Horizon--------------"

Menu "Survey`Import`Horizons`Ascii`Geometry 2D"
Input "Input ASCII File" "$EXPORTDIR$/Horizon2D-Export.asc" 
Combo "Select Line Set" "LS 5k"
Button "Add new"
Input "Name" "Horizon2D-Import"
#Ok
ListClick "Select Data" "Horizon2D-Import" 
Button "Define Format*"
Input "Horizon2D-Import" "col:5"
Ok
Button "Import"

Comment "----------Importing 2D Seismic Data---------------"

Menu "Survey`Import`Seismics`SEG-Y"
Window "SEG-Y tool"
Input "Input SEG-Y file" "$EXPORTDIR$/Seis2D-Export.sgy" 
Ok
Window "Determine SEG-Y*"
Ok
Window "SEG-Y Scan"
Input "Line name" "Seis2D"
Button "Select Store in Line Set" 
Input "Name" "Test-Import"
Input "Attribute" "Seis2D-Import"
Ok
Ok

Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Test-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Manage`Horizons 2D"
ListClick "Objects list" "Horizon2D-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Manage`Picksets"
ListClick "Objects list" "Pickset2D-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Menu "Survey`Select/Setup"
Ok

End

