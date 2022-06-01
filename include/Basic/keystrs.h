#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002

 RCS:		$Id$
________________________________________________________________________

-*/


#include "gendefs.h"
#include "fixedstring.h"

/*!
\brief is used for defining key strings that are 'global'.

The idea is also that we get some uniformity in how we read/write things
from/to file. Thus, if you suspect a string is rather common, try to find
something similar here first.

Also, some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    inline FixedString All()		{ return "All"; }
    inline FixedString Ascii()		{ return "Ascii"; }
    inline FixedString Attribute()	{ return "Attribute"; }
    inline FixedString Attribute2D()	{ return "Attribute2D"; }
    inline FixedString Attributes()	{ return "Attributes"; }
    inline FixedString Azimuth()	{ return "Azimuth"; }
    inline FixedString Binary()		{ return "Binary"; }
    inline FixedString Body()		{ return "Body"; }
    inline FixedString Category()	{ return "Category"; }
    inline FixedString Chain()		{ return "Chain"; }
    inline FixedString Color()		{ return "Color"; }
    inline FixedString CoordSys()	{ return "Coordinate System"; }
    inline FixedString Component()	{ return "Component"; }
    inline FixedString Components()	{ return "Components"; }
    inline FixedString Content()	{ return "Content"; }
    inline FixedString Crossline()	{ return "Cross-line"; }
    inline FixedString Cube()		{ return "Cube"; }
    inline FixedString Data()		{ return "Data"; }
    inline FixedString DataRoot()	{ return "Data Root"; }
    inline FixedString DataStorage()	{ return "Data storage"; }
    inline FixedString DataType()	{ return "DataType"; }
    inline FixedString DataSet()	{ return "DataSet"; }
    inline FixedString Date()		{ return "Date"; }
    inline FixedString DateTime()	{ return "Date/Time"; }
    inline FixedString Default()	{ return "Default"; }
    inline FixedString Depth()		{ return "Depth"; }
    inline FixedString DepthUnit()	{ return "Depth-Unit"; }
    inline FixedString Desc()		{ return "Description"; }
    inline FixedString Distribution()	{ return "Distribution"; }
    inline FixedString EmptyString()	{ return ""; }
    inline FixedString Factor()		{ return "Factor"; }
    inline FixedString Fault()		{ return "Fault"; }
    inline FixedString FaultStick()	{ return "FaultStick"; }
    inline FixedString FileName()	{ return "File name"; }
    inline FixedString Filter()		{ return "Filter"; }
    inline FixedString FloatUdf()	{ return "1e30"; }
    inline FixedString Format(int n=1)	{ return n<2 ? "Format" : "Formats"; }
    inline FixedString Geometry()	{ return "Geometry"; }
    inline FixedString GeomSystem()	{ return "GeomSystem"; }
    inline FixedString GeomID()		{ return "GeomID"; }
    inline FixedString GeomIDs()	{ return "GeomIDs"; }
    inline FixedString Histogram()	{ return "Histogram"; }
    inline FixedString Horizon()	{ return "Horizon"; }
    inline FixedString Horizon2D()	{ return "Horizon2D"; }
    inline FixedString ID()		{ return "ID"; }
    inline FixedString IDs()		{ return "IDs"; }
    inline FixedString Input()		{ return "Input"; }
    inline FixedString Inline()		{ return "In-line"; }
    inline FixedString IOSelection()	{ return "I/O Selection"; }
    inline FixedString Is2D()		{ return "Is2D"; }
    inline FixedString Keys()		{ return "Keys"; }
    inline FixedString Level()		{ return "Level"; }
    inline FixedString Line()		{ return "Line"; }
    inline FixedString Lines()		{ return "Lines"; }
    inline FixedString LineStyle()	{ return "Line Style"; }
    inline FixedString LineKey()	{ return "Line key"; }
    inline FixedString LineName()	{ return "Line name"; }
    inline FixedString LineNames()	{ return "Line names"; }
    inline FixedString Local()		{ return "Local"; }
    inline FixedString Log()		{ return "Log"; }
    inline FixedString Logs()		{ return "Logs"; }
    inline FixedString LogFile()	{ return "Log file"; }
    inline FixedString MD(int n=1)	{ return n<2 ? "MD" : "MDs"; }
    inline FixedString Marker()		{ return "Marker"; }
    inline FixedString MarkerStyle()	{ return "Marker Style"; }
    inline FixedString Mode()		{ return "Mode"; }
    inline FixedString Model()		{ return "Model"; }
    inline FixedString Models()		{ return "Models"; }
    inline FixedString Name()		{ return "Name"; }
    inline FixedString Names()		{ return "Names"; }
    inline FixedString NewLine()	{ return "\n"; }
    inline FixedString No()		{ return "No"; }
    inline FixedString None()		{ return "None"; }
    inline FixedString NrFaults()	{ return "Nr Faults";}
    inline FixedString NrGeoms()	{ return "Nr Geometries";}
    inline FixedString NrItems()	{ return "Nr Items";}
    inline FixedString NrValues()	{ return "Nr Values"; }
    inline FixedString Object()		{ return "Object"; }
    inline FixedString Offset()		{ return "Offset"; }
    inline FixedString OffsetRange()	{ return "Offset Range"; }
    inline FixedString Order()		{ return "Order"; }
    inline FixedString Output()		{ return "Output"; }
    inline FixedString OutputID()	{ return "output_id"; }
    inline FixedString Pars()		{ return "Parameters"; }
    inline FixedString PickSet()	{ return "PickSet"; }
    inline FixedString PreStackSeis()	{ return "PreStack Seismic"; }
    inline FixedString Polygon()	{ return "Polygon"; }
    inline FixedString Position()	{ return "Position"; }
    inline FixedString Property()	{ return "Property"; }
    inline FixedString Python()		{ return "Python"; }
    inline FixedString Quiet()		{ return "quiet"; }
    inline FixedString Random()		{ return "Random"; }
    inline FixedString RandomLine()	{ return "RandomLine"; }
    inline FixedString Range()		{ return "Range"; }
    inline FixedString Region()		{ return "Region"; }
    inline FixedString Sampling()	{ return "Sampling"; }
    inline FixedString Scale()		{ return "Scale"; }
    inline FixedString Scaling()	{ return "Scaling"; }
    inline FixedString SeisID()		{ return "SeisID"; }
    inline FixedString SeisCubePositions() { return "Seismic Cube Positions"; }
    inline FixedString Selection()	{ return "Selection"; }
    inline FixedString SelectionStatus(){ return "Selection status"; }
    inline FixedString Series()		{ return "Series"; }
    inline FixedString Server()		{ return "Server"; }
    inline FixedString ServerNm()	{ return "Server_Name"; }
    inline FixedString Setup()		{ return "Setup"; }
    inline FixedString Shape()		{ return "Shape"; }
    inline FixedString Shortcuts()	{ return "Shortcuts"; }
    inline FixedString Shotpoint()	{ return "Shotpoint"; }
    inline FixedString Size()		{ return "Size"; }
    inline FixedString SliceType()	{ return "SliceType"; }
    inline FixedString Source()		{ return "Source"; }
    inline FixedString SpaceString()	{ return " "; }
    inline FixedString StackOrder()	{ return "Stack order"; }
    inline FixedString Stats()		{ return "Stats"; }
    inline FixedString Status()		{ return "Status"; }
    inline FixedString Steering()	{ return "Steering"; }
    inline FixedString Stored()		{ return "Stored"; }
    inline FixedString StratRef()	{ return "Strat Level"; }
    inline FixedString Subsample()	{ return "Subsample"; }
    inline FixedString Subsel()		{ return "Subsel"; }
    inline FixedString Surface()	{ return "Surface"; }
    inline FixedString Survey()		{ return "Survey"; }
    inline FixedString SurveyID()	{ return "Survey ID"; }
    inline FixedString Table()		{ return "Table"; }
    inline FixedString Target()		{ return "Target"; }
    inline FixedString Targets()	{ return "Targets"; }
    inline FixedString TermEm()		{ return "Terminal Emulator"; }
    inline FixedString Thickness()	{ return "Thickness"; }
    inline FixedString ThreeD()		{ return "3D"; }
    inline FixedString Time()		{ return "Time"; }
    inline FixedString TimeRange()	{ return "Time Range"; }
    inline FixedString Title()		{ return "Title"; }
    inline FixedString TmpStor()	{ return "Temporary storage location"; }
    inline FixedString TraceNr()	{ return "Trace number"; }
    inline FixedString TraceKey()	{ return "Trace key"; }
    inline FixedString Transparency()	{ return "Transparency"; }
    inline FixedString TVD(int n=1)	{ return n<2 ? "TVD" : "TVDs"; }
    inline FixedString TVDSS()		{ return "TVDSS"; }
    inline FixedString TwoD()		{ return "2D"; }
    inline FixedString TWT()		{ return "TWT"; }
    inline FixedString Type()		{ return "Type"; }
    inline FixedString Types()		{ return "Types"; }
    inline FixedString Undef()		{ return "Undefined"; }
    inline FixedString Unit()		{ return "Unit"; }
    inline FixedString Units()		{ return "Units"; }
    inline FixedString User()		{ return "User"; }
    inline FixedString Value()		{ return "Value"; }
    inline FixedString Values()		{ return "Values"; }
    inline FixedString ValueRange()	{ return "Value Range"; }
    inline FixedString Version()	{ return "Version"; }
    inline FixedString Volume()		{ return "Volume"; }
    inline FixedString WaveletID()	{ return "Wavelet ID"; }
    inline FixedString Weight()		{ return "Weight"; }
    inline FixedString Well()		{ return "Well"; }
    inline FixedString X()		{ return "X"; }
    inline FixedString XCoord()		{ return "X-Coord"; }
    inline FixedString XCoords()	{ return "X-Coords"; }
    inline FixedString XOffset()	{ return "X Offset"; }
    inline FixedString XRange()		{ return "X range"; }
    inline FixedString Y()		{ return "Y"; }
    inline FixedString Y2()		{ return "Y2"; }
    inline FixedString YCoord()		{ return "Y-Coord"; }
    inline FixedString YCoords()	{ return "Y-Coords"; }
    inline FixedString YOffset()	{ return "Y Offset"; }
    inline FixedString YRange()		{ return "Y range"; }
    inline FixedString Yes()		{ return "Yes"; }
    inline FixedString Z()		{ return "Z"; }
    inline FixedString ZCoord()		{ return "Z-Coord"; }
    inline FixedString ZDomain()	{ return "Z Domain"; }
    inline FixedString ZRange()		{ return "Z range"; }
    inline FixedString ZSlice()		{ return "Z-slice"; }
    inline FixedString ZStep()		{ return "Z step"; }
    inline FixedString ZUnit()		{ return "Z-Unit"; }
    inline FixedString ZValue()		{ return "Z value"; }

    // Stats
    inline FixedString Average()	{ return "Average"; }
    inline FixedString Maximum()	{ return "Maximum"; }
    inline FixedString Median()		{ return "Median"; }
    inline FixedString Minimum()	{ return "Minimum"; }
    inline FixedString StdDev()		{ return "StdDev"; }
    inline FixedString RMS()		{ return "RMS"; }
    inline FixedString Sum()		{ return "Sum"; }
    inline FixedString Variance()	{ return "Variance"; }

    // (Horizontal) position selection in the survey
    inline FixedString InlRange()	{ return "In-line range"; }
    inline FixedString FirstInl()	{ return "First In-line"; }
    inline FixedString LastInl()	{ return "Last In-line"; }
    inline FixedString StepInl()	{ return "Step In-line"; }
    inline FixedString StepOut()	{ return "Stepout"; }
    inline FixedString StepOutInl()	{ return "Stepout In-line"; }
    inline FixedString CrlRange()	{ return "Cross-line range"; }
    inline FixedString FirstCrl()	{ return "First Cross-line"; }
    inline FixedString LastCrl()	{ return "Last Cross-line"; }
    inline FixedString StepCrl()	{ return "Step Cross-line"; }
    inline FixedString StepOutCrl()	{ return "Stepout Cross-line"; }
    inline FixedString FirstZ()		{ return "First Z"; }
    inline FixedString LastZ()		{ return "Last Z"; }
    inline FixedString StepZ()		{ return "Step Z"; }
    inline FixedString FirstTrc()	{ return "First Trace"; }
    inline FixedString LastTrc()	{ return "Last Trace"; }
    inline FixedString StepTrc()	{ return "Step Trace"; }
    inline FixedString TrcRange()	{ return "Trace Range"; }

    // History of objects
    inline FixedString CrBy()		{ return "Created.By"; }
    inline FixedString CrAt()		{ return "Created.At"; }
    inline FixedString CrFrom()		{ return "Created.From"; }
    inline FixedString CrInfo()		{ return "Created.Info"; }
    inline FixedString ModBy()		{ return "Modified.By"; }
    inline FixedString ModAt()		{ return "Modified.At"; }

    // Not used?
    inline FixedString CrFtPolygonDir() { return "FaultPolygonPath"; }
    inline FixedString StrFtPolygon()   { return "FaultPolygons"; }

};
