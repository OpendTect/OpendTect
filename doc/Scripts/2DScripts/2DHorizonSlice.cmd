dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Comment "-------Creating 2D-Horizon Slice---------"
Case Insensitive
Menu "Processing`Create output using Horizon`Horizon slice"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Button "Select Calculate along*"
ListClick "Objects list" "Horizon1" Double
Button "Select Trace subsel*"
Ok
Button "One line only" On
Combo "One line only" "i5007"
Input "Z Interval Start" -20
Input "Z Interval Stop" 20
Button "Select Store in Line*" 
Input "Name" "LS 5k slice"
Ok
Button "Proceed"
Ok

Sleep 5


