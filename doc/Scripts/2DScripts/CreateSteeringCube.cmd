dTect V3.2
OpendTect commands
Mon May 28 11:36:39 2008
!

Comment "-------------Creating Steering Cube-------------"

Case Insensitive

Menu "Processing`Steering`Create"
Button "Select Input Line Set"
ListClick "Objects list" "LS 5k" Double
Button "Select Trace subsel*"
Input "Number start" 238
Input "Number stop" 1200
Input "Number step" 4
Input "Z start" 0
Input "Z stop" 5000
Ok
Button "One line only" On
Combo "One line only" "i5007"
Button "Select Output Steering"
Input "Name" "Demo-Steer"
Input "Attribute" "test steering"
Ok
Combo "Steering Algorithm" "BG Fast*"
Button "Proceed"
iii
Ok
Sleep 95

Menu "Survey`Manage`Seismics"
ListClick "Objects list" "Demo-Steer"
Button "Remove this*"
Button "Yes"
Button "Dismiss"

End


