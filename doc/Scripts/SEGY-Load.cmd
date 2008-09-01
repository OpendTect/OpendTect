dTect V3.2
OpendTect commands
Mon May 21 11:36:39 2008
!

Case Insensitive

Comment "----------Loading Survey parameters-------------"

Menu "Survey`Select/Setup"
Button "New"

Input "Survey directory*" "TestTutorial"
Input "Full Survey name" "TestSurvey"
Input "Location on disk" "$BASEDIR$"
Combo "Survey type" "Only 3D"
Comment "------------"

Combo "Input parameters" "Scan SEG-Y*"
Button "Go"
# CHANGE_FILEPATH
Input "Input file" "$IMPORTDIR$/tut.sgy"
Input "X-coord byte" 81
Input "Y-coord byte" 85
Ok
Ok
Ok

#Importing SEG-Y data or Converting SEG-Y to CBVS_format

Comment "---------Importing SEG-Y data-------------"

Menu "Survey`Import`Seismics`SEG-Y`3D"
#Button "Define SEG-Y*"
Button "Select Volume*"
Combo "Volume subselection" "All"
#Input "Inline Start" 300
#Input "Inline stop" 305
Ok

Button "Select Output Cube"
Input "Name" "test-tut-survey"
Ok

Ok
Ok

Comment "---------Displaying the Loaded SEG-Y data-----------"

TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 128 
Ok
Button "Stored"
ListClick "Select Data" 1 Double

Menu "View`Z-scale"
Input "Z scale value" 15
Button "Fit to scene"
Ok

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/tut-segy-load.jpg"
Ok
Menu "Survey`Select/Setup"
Button "Remove"
Button "Yes"
ListClick "Select Data" "F3_Demo_for_Test"
Ok

End
