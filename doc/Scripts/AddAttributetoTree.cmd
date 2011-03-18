dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Comment "----------Tour of  All Tree items----------"

Case Insensitive

TreeMenu "Scene 1" "Properties"
Button "Background color"
ColorOK Cyan 4
Ok
TreeMenu "Scene 1" "Properties"
Button "Background color"
ColorOK Black 4
Ok

Comment "------Adding MedianDipFilteredSeismic to Trree item Inline----"

TreeMenu "Inline" "Add"
TreeMenu "inline`425" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
#Color_Apply
Combo "Table selection" "Grey scales" 
TreeMenu "Inline`*`Median Dip*" "Show Amplitude Spectrum" 
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Inl320-AmpSpect.png" CurWin
Close
TreeMenu "Inline`*`Median Dip*" "Show Histogram"
Window "Data Statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Inl320-Histogram.png" CurWin
Close

Comment "------Adding MedianDipFilteredSeismic to Trree item Crossline----"

TreeMenu "Crossline" "Add"
Input "Slice position" 825
Button "Stored"
#Change Seismic cube
ListClick "Select Data" "Median Dip*" Double

Combo "Table selection" "Altimetric"
TreeMenu "Crossline`*`Median Dip*" "Show Amplitude Spectrum"
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Crl825-AmpSpect.png" CurWin
Close
TreeMenu "Crossline`*`Median Dip*" "Show Histogram"
Window "Data Statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Crl825-Histogram.png" CurWin
Close

Comment "------Adding MedianDipFilteredSeismic to Trree item TimeSlice----"

TreeMenu "Timeslice" "Add"
Input "Time" 925
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
Combo "Table selection" "Brown 4grades"
TreeMenu "Timeslice`*`Median Dip*" "Show Amplitude Spectrum"
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_TS925-AmpSpect.png" CurWin
Close
TreeMenu "Timeslice`*`Median Dip*" "Show Histogram"
Window "Data Statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_TS925-Histogram.png" CurWin
Close

Comment "------Adding MedianDipFilteredSeismic to Trree item Volume----"

TreeMenu "Volume" "Add"
TreeMenu "Volume`*" "Position"

Input "Inl Start" 300
Input "Inl Stop" 400
Input "Crl Start" 750
Input "Crl Stop" 900
Input "Z Start" 800
Input "Z Stop" 1300
Ok
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
TreeButton "Volume`Median Dip*`Volren" On
Sleep 3
#Combo "Table selection" "Flames"
TreeMenu "Volume`*" "Show Amplitude Spectrum"
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Vol-AmpSpect.png" CurWin
Close
TreeMenu "Volume`Median Dip*" "Show Histogram"
Window "Data Statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Vol-Histogram.png" CurWin
Close

Comment "-----Adding MedianDipFilteredSeismic to Trree item RandomLine----"

TreeMenu "Random line" "New"
TreeMenu "Random line`Random Line 1" "Edit nodes"
TableFill "BinID Table" 1 1 200
TableFill "BinID Table" 1 2 330
TableFill "BinID Table" 2 1 750
TableFill "BinID Table" 2 2 950
TableFill "BinID Table" 3 1 1000
TableFill "BinID Table" 3 2 1200
Ok
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
TreeMenu "Random line`*`Median Dip*" "Show Amplitude Spectrum"
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Randomline-AmpSpect.png" CurWin
Close
TreeMenu "Random line`*`Median Dip*" "Show Histogram"
Window "Data statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Randomline-Histogram.png" CurWin
Close
Combo "Table selection" "Red-White-Blue"

Comment "------------Loading PickSet- ---------------"

TreeMenu "PickSet" "Load"
ListClick "Objects list" 1 Double

Comment "------------Loading Horizon-------------------"

TreeMenu "Horizon" "Load"
ListClick "Objects list" 1 Double

TreeMenu "Horizon`*" "Add attribute"
TreeMenu "Horizon`*`<right-click>" "Select Att*`Stored*`Median Dip*"
TreeMenu "Horizon`*`Median*" "Show Histogram"
Window "Data statistics"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Horizon-histogram.png"
Close
TreeMenu "Horizon`*`Median*" "Show Amplitude*"
Window "Amplitude Spectrum*"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_Horizon-AmpSpect.png"
Close
TreeMenu "Horizon`*" "Tracking`Wireframe"

Comment "------------Loading Fault------------------"
TreeMenu "Fault" "Load"
ListClick "Objects list" "TestFault" Double


Comment "------------Loading FaultStickSet------------------"
TreeMenu "FaultStickSet" "Load"
ListClick "Objects list" 1 Double

Comment "--------------Loading Well---------------"

TreeMenu "Well" "Load"
ListClick "Objects list" "F03-2" Double
TreeMenu "Well`*" "Create attribute log"
Button "Select Input*"
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
Input "Log name" "Testlog"
Ok
TreeMenu "Well`*" "Properties" 
Window "Well display*"
Tab "Well display*" "Left Log"
Combo "Select log" 2
Button "log filled" Off
Tab "Well display*" "Right Log"
Combo "Select log" 3
Button "log filled" On
Tab "Well display*" "Track"
Input "Size" 2
Button "Line color"
ColorOk Red 2
Button "Dismiss"

Comment "--------------Annotations-----------------"

TreeExpand "Annotations"
TreeMenu "Annotations`Arrows" "Load"
ListClick "Objects list" "downlap" Double
TreeMenu "Annotations`Arrows`downlap" "Properties"
Button "Line color"
ColorOK Brown 4
Input "Width" 3
Slider "Size" 80
Sleep 3
Ok

Wheel "vRotate" 40
Wheel "hRotate" 20
Slider "Zoom Slider" 25
Sleep 8
Menu "Survey`Session`Save"
Input "Name" "DemoTreeItems"
Ok
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_AllTreeItems.png" ODMain
Sleep 4

Wheel "hRotate" -20
Wheel "vRotate" -40
Slider "Zoom Slider" 29

TreeMenu "Inline`*" "Remove"
TreeMenu "Crossline`*" "Remove"
TreeMenu "Timeslice`*" "Remove"
TreeMenu "Volume`*" "Remove"
TreeMenu "Random line`*" "Remove"
TreeMenu "PickSet`*" "Remove"
TreeMenu "Horizon`*" "Remove"
TreeMenu "Fault`*" "Remove"
TreeMenu "FaultStickSet`*" "Remove"
TreeMenu "Well`*" "Remove"

Button "Manage well data"
ListClick "Objects list" "F03-2"
ListClick "Available logs" "Testlog"
Button "Remove selected log"
Button "Yes"
Button "Dismiss"

TreeButton "Annotations`Arrows`*" Off
Menu "Survey`Session`Restore"
ListClick "Objects list" "DemoTreeItems"
Button "Remove this object"
Button "Yes"
Button "Cancel"

Comment "-----------RandomLine Create From wells-----------------"

TreeMenu "Random line" "Generate`From Wells"
Window "Create Random*"
ListSelect "Available Wells" 1 4 On
Button "Move right"
Button "Select Output Random*"
Input "Name" "RanLineOnWells"
Ok
Button "Display Random Line*" On
Ok
TreeMenu "Random line`*`*" "Select Attribute`Stored Cubes`Median Dip*"

TreeMenu "Well" "Load"
ListSelect "Objects list" 1 4 On
Ok

Wheel "hRotate" 30
Wheel "vRotate" 40
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_wells-atRandomLine.png" ODMain
Sleep 3
Wheel "vRotate" -40
Wheel "hRotate" -30

TreeMenu "Random line`*" "Remove"
TreeMenu "Well" "Remove all items"
Button "Yes"

TreeMenu "Random line" "Generate`From Wells"
Window "Create Random*"
Button "Select Output*"
ListClick "Objects list" "RanLineOnWells"
Button "Remove this*"
Button "Yes"
Button "Cancel"
Button "Cancel"

End
