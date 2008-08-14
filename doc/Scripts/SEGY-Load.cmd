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
Input "Location on disk" "$BASEDIR$/F3_Demo_for_Test"
Combo "Survey type" "Only 3D"
Comment "------------"

Combo "Input parameters" "Scan SEG-Y*"
Button "Go"

# CHANGE_FILEPATH
Input "Input file" "/d12/nageswara/dev/dgb/data/tut.sgy"
Input "X-coord byte" 81
Input "Y-coord byte" 85

Ok
Ok
Ok

#Importing SEG-Y data or Converting SEG-Y to CBVS_format

Comment "---------Importing SEG-Y data-------------"

Menu "Survey`Import`Seismics`SEG-Y`3D"
Button "Define SEG-Y*"
# CHANGE_FILEPATH
Input "Input file" "/d12/nageswara/dev/dgb/data/tut.sgy"
Ok
Button "Select Volume*"
Combo "Volume subselection" "All"
Ok
Button "Discard"

Button "Select Output Cube"
Input "Name" "test-tut-survey"
Ok

Ok
Ok

Comment "---------Displaying the Loaded SEG-Y data-----------"

TreeMenu "Inline" "Add"
TreeMenu "Inline`127`<right-click>" "Select Attribute`Stored Cubes`test-tut-survey"

Menu "View`Z-scale"
Input "Z scale value" 15
Ok
Wheel "hRotate" 90

End
