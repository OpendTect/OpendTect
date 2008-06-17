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
Input "Location on disk" "$DATADIR$/Misc/"
Combo "Survey type" "Only 3D"
Comment "------------"

Combo "Input parameters" "Scan SEG-Y*"
Button "Go"

Input "Input file" "/d12/nageswara/dev/dgb/data/tut.sgy"
Input "X-coord byte" 81
Input "Y-coord byte" 85

Ok
Ok

#If TestTutorial Survey is already existed then TestTutorial-1 is created.

OnError Continue
Button "Ok"
Input "Survey directory*" "TestTutorial-1"
Ok
Ok
OnError Stop

#Importing SEG-Y data or Converting SEG-Y to CBVS format

Comment "---------Importing SEG-Y data-------------"

Menu "Survey`Import`Seismics`SEG-Y`3D"
Button "Define SEG-Y*"
Input "Input file" "/d12/nageswara/dev/dgb/data/tut.sgy"
Input "In-line byte" 9
Input "Cross-line byte" 21
Ok
Button "Select Volume*"
Combo "Volume subselection" "All"
Ok
Button "Discard"

Button "Select Output Cube"
Input "Name" "test-tut-survey"
Ok

OnError Continue
Button "Yes"
Ok
Ok
OnError Stop
Comment "---------Displaying the Loaded SEG-Y data-----------"

TreeMenu "Inline" "Add"
TreeMenu "Inline`127`<right-click>" "Select Attribute`Stored Cubes`test-tut-survey"

Menu "View`Z-scale"
Input "Z scale value" 15
Ok
Wheel "hRotate" 90

End
