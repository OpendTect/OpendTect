dTect V3.2
OpendTect commands
Mon Jun 18 11:36:39 2008
!

Case Insensitive
Comment "----------Exporting Seismics----------"
Menu "Survey`Export`Seismics`SEG-Y`3D"
Button "Select Input Cube"
ListClick "Objects list" "Median Dip Filtered*" Double
Button "Select Volume subsel*"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Button "define Text header"
Ok
Input "Output SEG-Y file" "$EXPORTDIR$/SeisExport.sgy"
Ok
Ok

Comment "----------Exporting Horizons---------------"
Menu "Survey`Export`Horizons`Ascii 3D"
Button "Select Input Surface"
ListClick "Objects list" 1 Double
Button "Yes"
Input "Output Ascii file" "$EXPORTDIR$/HorizonExport.asc"
Ok
Button "Dismiss"

Comment "-----------Exporting PickSet--------------------"
Menu "Survey`Export`Picksets"
Button "Select Input PickSet"
ListClick "Objects list" 1 Double
Input "Output Ascii file" "$EXPORTDIR$/PicksetExport.asc"
Button "Go"
Ok
Button "Dismiss"

Comment "------------Exporting Well-------------------"
Menu "Survey`Manage`Wells"
ListClick "Objects list" 2
ListClick "Available logs" 2
Button "Export log"
Input "Output file" "$EXPORTDIR$/WellExport.dat"
Ok
Button "Dismiss"

Comment "-----------Exporting Fault--------------------"
Menu "Survey`Export`Faults`Ascii"
Button "Select Input Fault"
ListClick "Objects list" "TestFault" Double
Input "Output Ascii file" "$EXPORTDIR$/FaultExport.asc"
Ok

Comment "-----------Exporting FaultSickSet--------------------"
Menu "Survey`Export`FaultStickSets`Ascii"
Button "Select Input FaultStickSet" 
ListClick "Objects list" "FaultStickSet" Double
Input "Output Ascii file" "$EXPORTDIR$/FaultSickSetExport.asc"
Ok

End

