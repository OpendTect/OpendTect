dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "--------------Create Seismic output---------------"

Case Insensitive
Menu "Processing`Create Seismic output"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" 5 Double
Button "Select Volume subsel*"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Button "Select Output Cube"
Input "Name" "SeismicOutputCreate"
Ok
Button "Proceed"
Ok
Sleep 10

TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok
Button "Stored"
ListClick "Select Data" "SeismicOutputCreate" Double

Menu "Survey`Manage`Seismics"
ListClick "Objects list" "SeismicOutputCreate"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

TreeMenu "Inline`*" "Remove"

# Parameter id is not correct error
# is coming After running this script second time
# to rectify this problem select same survey again

Menu "Survey`Select/Setup"
Sleep 1
Ok

End
