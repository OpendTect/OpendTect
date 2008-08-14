dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Importing Seismic Data--------------"

Menu "Survey`Import`Seismics`SEG-Y`3D"
Button "Define SEG-Y Input" 
Input "Input file" "$SCRIPTSDIR$/Export/SeisExpo.sgy"
Ok
Button "Select Volume sub*"
Input "Inline start" 320
Input "Inline stop" 320
#Input "Crossline start" 300
#Input "Crossline stop" 1200
#Input "Z start" 500
#Input "Z stop" 1500
Ok
Button "Select Output Cube"
Input "Name" "SeisImport"
Ok
Ok
Ok
Comment "-------------Importing Horizon----------"

Menu "Survey`Import`Horizons`Ascii`Geometry 3D"
Input "Input ASCII File" "$SCRIPTSDIR$/Export/HorizonExpo.asc"
Button "Define Format Def*" 
Ok
Button "Select Horizon" 
Input "Name" "HorozonImport.hor"
Ok
Ok

Comment "----------------Importing Pickset------------"
Input "Input Ascii file" "$SCRIPTSDIR$/Export/pickset.asc"
Button "Define Format definition" 
Combo "Unit" "Seconds"
Ok
Button "Select Output PickSet"
Input "Name" "Pickset-export"
Ok
Button "Go"
Ok
Button "Dismiss"

Comment "----------------Importing Fault------------"
Menu "Survey`Import`Faults`Ascii 3D"
Input "Input ascii file" "/$SCRIPTSDIR$/Export/ExpoTestFault.asc"
Button "Define Format defi*"
Ok
Button "Select Output Fault"
Input "Name" "TestFaultImport.flt"
Ok
OnError Continue
Ok
Button "Go"
Ok
OnError Stop

Button "Dismiss"

End
