dTect V3.2
OpendTect commands
Mon May 28 11:36:39 2008
!

Comment "-------------Creating Steering Cube-------------"

Case Insensitive

Menu "Processing`Steering`Create"
Button "Select Input Line Set"
ListClick "Objects list" 1 Double
Button "Select Trace subsel*"
#Window "Positions"
Ok
Button "One line only" On
Combo "One line only" "i5007"
Button "Select Output Steering"
Input "Name" "Demo-Steer"
Input "Attribute" "Steering"
Ok
Combo "Steering Algorithm" "BG Fast*"
Button "Proceed"
Ok
Ok

End

