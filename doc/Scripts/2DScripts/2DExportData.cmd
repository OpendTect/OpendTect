dTect V3.2
OpendTect commands
Mon Jun 18 11:36:39 2008
!

Case Insensitive
Comment "----------Exporting 2D seismic Data----------"

Menu "Survey`Export`Seismics`SEG-Y`2D"
Button "Select Input Line Set"
ListClick "Objects list" "LS 5k" double
Combo "Line name" "i5007"
Input "Select Output SEG-Y file" "$EXPORTDIR$/Seis2D-Export.sgy"
Ok
Comment "----------Exporting 2D Horizon----------"

Menu "Survey`Export`Horizons`Ascii 2D"
Combo "Select 2D Horizon" 1
ListClick "Select lines" "i5007"
Input "Output Ascii file" "$EXPORTDIR$/Horizon2D-Export.asc"
Ok

Comment "----------Exporting PickSet-------------"

Menu "Survey`Export`Picksets"
Button "Select Input PickSet"
ListClick "Objects list" 1 Double
Input "Output Ascii file" "$EXPORTDIR$/Pickset2D-Export.asc"
Ok
Ok
Button "Dismiss"

End

