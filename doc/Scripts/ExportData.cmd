dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Exporting Seismics----------"

Menu "Survey`Export`Seismics`SEG-Y`3D"
Button "Select Input Cube"
ListClick "Objects list" "Median Dip*" Double
Button "Select Volume subsel*"
Ok
Button "define SEG-Y Output"
Input "Output file" "/$SCRIPTSDIR$/Export/SeisExpo.sgy"
Ok
OnError Continue
Button "Yes"
Ok
OnError Stop

Comment "----------Exporting Horizons---------------"

Menu "Survey`Export`Horizons`Ascii"
Button "Select Input Surface"
ListClick "Objects list" 2 Double
Input "Output Ascii file" "/$SCRIPTSDIR$/Export/HorizonExpo"
Ok
OnError Continue
Button "Yes"
OnError Stop
Sleep 1
Button "Dismiss"

Comment "------------Exporting Fault----------------"

Menu "Survey`Export`Faults`Ascii"
Button "Select Input Fault"
ListClick "Objects list" "1000 between 4 and 8" Double
Input "Output Ascii file" "/$SCRIPTSDIR$/Export/FaultExpo"
Ok
OnError Continue
Button "Yes"
Ok
OnError Stop


End
