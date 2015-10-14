dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Case Insensitive

Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "New attribute set"
Combo "Attribute group" "<All>"
Button "Save on OK" off

Comment "-----------Coherency Attribute----------------------"


Combo "Attribute type" "Coherency"
Button "Select*"
ListClick "Select Data" 1 Double
Input "Z start" -10
Input "Z stop" 10
Input "Maximum dip*" 50
Input "Delta dip*" 5
Input "Attribute name" "CoherencyAttrib"
Comment "----------------------------"

Comment "----------Convolve Attribute---------"

Combo "Attribute type" "Convolve"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Combo "Filter type" 1
Input "Filter size" 5
#Button "Sphere" 

Input "Attribute name" "ConvolveAttrib"
Comment "---------------------"

Comment "----------Curvature Attribute-----------"

Combo "Attribute type" "Curvature"
Spin "Stepout" 2
Button "Yes"  
Button "Select Steering Data"
ListClick "Objects list" 1 Double
Input "Attribute name" "CurvatureAttrib"
Comment "-----------------------"

Comment "----------DipAngle Attribute----------"

Combo "Attribute type" "Dip"
Button "Select Steering Data"
ListClick "Objects list" 1 Double
Input "Attribute name" "DipAttrib"
Comment "-----------------------------"

Comment "----------Dip Attribute--------"

Combo "Attribute type" "Dip Angle"
Button "Select Input Dip Attribute"
ListClick "Select Data" "DipAttrib" Double
Button "Yes"
Input "Velocity" 2500
Input "Attribute name" "DipAngleAttrib"
Comment "------------------------"

Comment "----------Energy Attribute-----------"

Combo "Attribute type" "Energy"
Button "Select Input Data"
Button "Stored"
ListClick "Select Data" 1 Double
Button "No"
Input "Attribute name" "EnergyAttrib"
Comment "------------------------------"

Comment "----------Event Attribute-------------"

Combo "Attribute type" "Event"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Button "Multiple events"
Combo "Event type" "Extremum"
Button "Next event"
Input "Attribute name" "EventAttrib"
Comment "-------------------------------"

Comment "----------Frequency Attribute-------------"

Combo "Attribute type" "Frequency"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Input "Z Start" -10
Input "Z Stop" 10
Button "Yes"
Combo "Window/Taper" "Hamming"
Combo "Output" "Dominant frequency"
Input "Attribute name" "FrequencyAttrib"
Comment "----------------------------"


Comment "----------FrequencyFilter Attribute-----------"

Combo "Attribute type" "Frequency Filter"
Button "Select Input Data"
Button "Stored"
ListClick "Select Data" 1 Double
Button "ButterWorth"
Combo "Filter type" "HighPass"
Input "Min frequency" 13 
Input "Nr of poles" 10
Input "Attribute name" "FrequencyFilterAttrib"
Comment "-------------------------------"

Comment "----------Instantaneous Attribute---------"

Combo "Attribute type" "Instantaneous"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Combo "Output" "Phase"

Input "Attribute name" "InstantaneousAttrib"
Comment "---------------------------"

Comment "---------Position Attrinute------------"

Combo "Attribute Type" "Position"
Button "Select Input attribute"
Button "Stored"
ListClick "Select Data" 1 Double
Spin "Inl Stepout" 10
Input "Z start" 10
Input "Z stop" 50
Combo "Operator" "Min"
Button "Select Output Attribute"
Button "Stored"
ListClick "Select Data" 1 Double
Input "Attribute name" "PositionAttrib"
Comment "----------------------------"

Comment "---------ReferenceShift Attribute---------"

Combo "Attribute type" "Reference shift"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Spin "Inl Shift" 5
Input "Z Shift" 10
Button "No"
Input "Attribute name" "ReferenceShiftAttrib"
Comment "--------------------"

Comment "----------Scaling------------"

Combo "Attribute type" "Scaling"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Combo "Type" 1
Input "n" 3
Input "Attribute name" "ScalingAttrib" 
Comment "-------------------------------"

Comment "---------Similarity Attribute----------------"

Combo "Attribute type" "Similarity"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Input "Z start" -10
Input "Z stop" 10
Combo "Extension" "Mirror 180*"
Spin "Trc1 Inl" 5
Spin "Trc2 Inl" 5
Combo "Output statistic" "Average"
Input "Attribute name" "SimilarityAttrib"
Comment "------------------------------------"

Comment "-----------SpectralDecomp Attribute-------------"

Combo "Attribute type" "Spectral Decomp"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Button "FFT"
Input "Z start" -10
Input "Z Stop" 10
Spin "Output frequency*" 4
Spin "step" 2
Input "Attribute name" "SpectralDecompAttrib"
Comment "---------------------------"

Comment "-----------VelocityFanFilter Attribute------------"

Combo "Attribute type" "Velocity Fan Filter"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Spin "Filter size" 3
Combo "Velocity to pass" "High"
Input "Min velocity" 0
Button "No"
Input " Taper length*" 20
Input "Attribute name" "VelocityFanFilterAttrib"
Comment "-------------------------------------"

Comment "------------Volume Statistics-------------"

Combo "Attribute type" "Volume Statistics"
Button "Select*"
Button "Stored"
ListClick "Select Data" 1 Double
Input "Z start" -10
Input "Z stop" 10
Spin "Inl Stepout" 5
Spin "Min nr of valid traces" 5
Combo "output statistic" "Average"
Input "Attribute name" "VolumeStatisticsAttrib"
Comment "--------------------------------------"

Ok

End
~       


