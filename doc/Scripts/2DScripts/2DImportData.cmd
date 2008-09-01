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
ListClick "Select Data" "Top hor" 
Button "Define Format*"
Input "Top hor" "col:5"
Ok
Button "Import"

Comment "----------Importing 2D Seismic Dat---------------"

Menu "Survey`Import`Seismics`SEG-Y`2D"
Button "Define SEG-Y input"
Input "Input file" "$EXPORTDIR$/Seis2D-Export.sgy" 
Ok
Input "Line name" "Seis2D"
Button "Select Store in Line Set" 
Input "Name" "LS 5k"
Input "Attribute" "Seis2D-Import"
Ok
Ok

Menu "Survey`Select\Setup"
Ok

End
