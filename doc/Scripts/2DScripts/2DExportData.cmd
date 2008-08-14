dTect V3.2
OpendTect commands
Mon Jun 18 11:36:39 2008
!

Case Insensitive
Comment "----------Exporting 2D seismic Data----------"

Menu "Survey`Export`Seismics`SEG-Y`2D"
Button "Select Input Line Set"
ListClick "Objects list" "LS 5k" double
Button "Define SEG-Y output"
Input "Select Output file" "$SCRIPTSDIR$/Export/Seis2DExpo.sgy"
Ok
Ok
Comment "----------Exporting 2D Horizon----------"

Menu "Survey`Export`Horizons`Ascii 2D"
Combo "Select 2D Horizon" 1
ListClick "Select lines" "i5007"
Input "Output Ascii file" "$SCRIPTSDIR$/Export/Horizon2DExp.asc"
Ok

Comment "----------Exporting PickSet-------------"

Menu "Survey`Export`Picksets"
Button "Select Input PickSet"
ListClick "Objects list" 1 Double
Input "Output Ascii file" "$SCRIPTSDIR$/Export/Pickset2DExp.asc"
Ok
Ok
Button "Dismiss"

End
