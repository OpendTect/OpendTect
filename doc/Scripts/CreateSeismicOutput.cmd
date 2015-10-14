dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "--------------Create Seismic output---------------"

Case Insensitive
Menu "Processing`Create Volume output`Cube"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "Median Dip Filtered*"  Double
Button "Select Volume subsel*"
Input "Inline start" 320
Input "Inline stop" 320
Input "Crossline start" 302
Input "Crossline stop" 750
Ok
Button "Select Output Cube"
Input "Name" "SeismicVolume-1"
Ok
Button "Proceed"
Ok
Sleep 6

Menu "Processing`Create Volume output`Cube"
Button "Select Quantity to output"
Button "Stored"
ListClick "Select Data" "Median Dip Filtered*"  Double
Button "Select Volume subsel*"
Input "Inline start" 320
Input "Inline stop" 320
Input "Crossline start" 751
Input "Crossline stop" 1248
Ok
Button "Select Output Cube"
Input "Name" "SeismicVolume-2"
Ok
Button "Proceed"
Ok
Sleep 6

Menu "Survey`Manage`Seismics"
Button "Merge blocks*"
ListClick "Input Cubes" "SeismicVolume-1" 
ListSelect "Input Cubes" "SeismicVolume-2" On
Button " Select Output Cube"
Input "Name" "NewSeismicVolume"
Ok
Ok
Sleep 2

ListClick "Objects list" "NewSeismicVolume"
Button "Rename this object"

Input "New name" "SeismicVolumeCreate"
Button "Copy cube"
Button "Select Input Cube"
ListClick "Objects list" "SeismicVolumeCreate" Double
Button "Select Volume*"
Ok
Button "Select Output Cube" 
Input "Name" "SeismicVolumeCreate-copy"
Ok
Ok
Ok

Button "Browse/edit*"
Window "Browse seismic*"
Button "Goto position"
Input "Crossline" 600
Ok
Button "Information"
Window "Selected Trace*"
Button "Dismiss"
Button "Switch to Crossline"
Button "Switch to Crossline"
Button "Move left"
Button "Move right"
Button "Show Wiggle"
Window "Browse*"
Button "Cancel"


ListClick "Objects list" "SeismicVolume-1"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "SeismicVolume-2"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "SeismicVolumeCreate"
Button "Remove this object"
Button "Yes"
ListClick "Objects list" "SeismicVolumeCreate-copy"
Button "Remove this object"
Button "Yes"

Button "Dismiss"

# Parameter id is not correct error
# is coming After running this script second time
# to rectify this problem select same survey again

Menu "Survey`Select/Setup"
Sleep 1
Ok

End

