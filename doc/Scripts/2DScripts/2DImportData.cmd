dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Importing Pickset--------------"

Menu "Survey`Import`Picksets"
Input "Input Ascii file" "$SCRIPTSDIR$/Export/Pickset2DExp.asc"
Button "Define Format def*"
Ok
Button "Select Output Pickset"
Input "Name" "Pickset2DImpo"
Ok
Button "Go"
Ok

Sleep 2
Button "Dismiss"

Comment "----------Importing 2D Horizon--------------"

Menu "Survey`Import`Horizons`Ascii`Geometry 2D"
Input "Input ASCII File" "$SCRIPTSDIR$/Export/Horizon2DExp.asc"
Combo "Select Line Set" "LS 5k"
Button "Add new"
Input "Name" "Top hor"
ListClick "Select Data" "Top hor" 
Button "Define Format*"
Input "Top hor" "col:5"
Ok
Button "Import"

Comment "----------Importing 2D Seismic Dat---------------"

Menu "Survey`Import`Seismics`SEG-Y`2D"
Button "Define SEG-Y input"
Input "Input file" "$SCRIPTSDIR$/Export/Seis2DExpo.sgy"
Ok
Input "Line name" "Seis2D"
Button "Select Store in Line Set" 
Input "Name" "LS 5k"
Input "Attribute" "seis-impo"
Ok
Ok

Menu "Survey`Select\Setup"
Ok

End
