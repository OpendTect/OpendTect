dTect V3.2
OpendTect commands
Mon May 21 11:36:39 2008
!

Case Insensitive

Comment "----------Loading Survey parameters-------------"

Menu "Survey`Select/Setup"
Button "New"
Input "Survey directory name" "TestTutorial"
Input "Full Survey name" "TestSurvey"
Input "Location on disk" "$BASEDIR$"
Combo "Survey type" "Only 3D"
Combo "SIPs" "Scan SEG-Y*"
Button "Go"
Comment "---------------------"

Window "SEG-Y tool"
Input "Input SEG-Y file(s)" "$IMPORTDIR$/tut.sgy"
Combo "File type" "3D Volume"
Ok
Window "SEG-Y Scan"
Button "Coordinates"
Tab "SEG-Y def*" "Coordinates"
Input "X-coord byte" 81
Input "Y-coord byte" 85
Ok
Ok
Ok
Button "Yes"

#Importing SEG-Y data or Converting SEG-Y to CBVS_format

Comment "---------Importing SEG-Y data-------------"

Window "SEG-Y Scan"
Button "Coordinates"
Tab "SEG-Y def*" "Coordinates"
Input "X-coord byte" 81
Input "Y-coord byte" 85
Button "Select Volume*"
Combo "Volume subselection" "All"
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
Input "Z stretch value" 15
Button "Fit to scene"
Ok
Wheel "hRotate" 90

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_tut-segy-load.jpg"
Ok

Menu "Survey`Select/Setup"
Button "Remove"
Button "Yes"
ListClick "Select Data" "F3_Demo_for_Test" Double

End
