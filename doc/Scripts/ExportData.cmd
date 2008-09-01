dTect V3.2
OpendTect commands
Mon Jun 18 11:36:39 2008
!

Case Insensitive
Comment "----------Exporting Seismics----------"

Menu "Survey`Export`Seismics`SEG-Y`3D"
Button "Select Input Cube"
ListClick "Objects list" "Median Dip*" Double
Button "Select Volume subsel*"
Ok
Button "define SEG-Y Output"
Input "Output file" "$EXPORTDIR$/SeisExport.sgy"
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

Comment "-----------Exporting Fault--------------------"

#Menu "Survey`Export`Faults`Ascii"
#Button "Select Input Fault"
#ListClick "Objects list" "TestFault" Double
#Input "Output Ascii file" "$EXPORTDIR$/FaultExport.asc"
#Ok

End
