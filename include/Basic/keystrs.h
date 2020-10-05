#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2002

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
    inline FixedString Attribute(int n=1) { return n<2 ? "Attribute"
						       : "Attributes"; }
    inline FixedString Attribute2D()	{ return "Attribute2D"; }
    inline FixedString Attributes()	{ return Attribute(mPlural); }
    inline FixedString Azimuth()	{ return "Azimuth"; }
    inline FixedString Binary()		{ return "Binary"; }
    inline FixedString Body()		{ return "Body"; }
    inline FixedString Category()	{ return "Category"; }
    inline FixedString Chain()		{ return "Chain"; }
    inline FixedString Color()		{ return "Color"; }
    inline FixedString ColTab()		{ return "Color Table"; }
    inline FixedString Coord()		{ return "Coordinate"; }
    inline FixedString CoordSys()	{ return "Coordinate System"; }
    inline FixedString Component(int n=1) {return n<2?"Component":"Components";}
    inline FixedString Content()	{ return "Content"; }
    inline FixedString Crossline(int n=1) { return n<2 ? "Cross-line"
						       : "Cross-lines"; }
    inline FixedString Cube(int n=1)	{ return n<2 ? "Cube" : "Cubes"; }
    inline FixedString Data()		{ return "Data"; }
    inline FixedString DataRoot()	{ return "Data Root"; }
    inline FixedString DefDataRoot()	{ return "Default DATA directory"; }
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
    inline FixedString Empty()		{ return OD::EmptyString(); }
    inline FixedString Factor()		{ return "Factor"; }
    inline FixedString Fault()		{ return "Fault"; }
    inline FixedString FileName()	{ return "File name"; }
    inline FixedString Filter()		{ return "Filter"; }
    inline FixedString Format(int n=1)	{ return n<2 ? "Format" : "Formats"; }
    inline FixedString FloatUdf()	{ return "1e30"; }
    inline FixedString Geometry()	{ return "Geometry"; }
    inline FixedString GeomSystem()	{ return "GeomSystem"; }
    inline FixedString GeomID(int n=1)	{ return n<2 ? "GeomID" : "GeomIDs"; }
    inline FixedString Horizon()	{ return "Horizon"; }
    inline FixedString ID(int n=1)	{ return n<2 ? "ID" : "IDs"; }
    inline FixedString Input()		{ return "Input"; }
    inline FixedString Inline(int n=1)	{ return n<2 ? "In-line" : "In-lines"; }
    inline FixedString IOSelection()	{ return "I/O Selection"; }
    inline FixedString Is2D()		{ return "Is2D"; }
    inline FixedString Isochore()	{ return "Isochore"; }
    inline FixedString Isochron()	{ return "Isochron"; }
    inline FixedString IsPS()		{ return "IsPS"; }
    inline FixedString Key(int n=1)	{ return n<2 ? "Key" : "Keys"; }
    inline FixedString Level()		{ return "Level"; }
    inline FixedString Line(int n=1)	{ return n<2 ? "Line" : "Lines"; }
    inline FixedString LineStyle()	{ return "Line Style"; }
    inline FixedString MD(int n=1)	{ return n<2 ? "MD" : "MDs"; }
    inline FixedString Marker()		{ return "Marker"; }
    inline FixedString MarkerStyle()	{ return "Marker Style"; }
    inline FixedString Mode()		{ return "Mode"; }
    inline FixedString Model()		{ return "Model"; }
    inline FixedString LineKey()	{ return "Line key"; }
    inline FixedString LineName()	{ return "Line name"; }
    inline FixedString Local()		{ return "Local"; }
    inline FixedString Log(int n=1)	{ return n < 2 ? "Log" : "Logs"; }
    inline FixedString LogFile()	{ return "Log file"; }
    inline FixedString Name(int n=1)	{ return n<2 ? "Name" : "Names"; }
    inline FixedString NewLine()	{ return "\n"; }
    inline FixedString No()		{ return "No"; }
    inline FixedString None()		{ return "None"; }
    inline FixedString NrFaults()	{ return "Nr Faults";}
    inline FixedString NrDone()		{ return "Nr Done"; }
    inline FixedString NrGeoms()	{ return "Nr Geometries";}
    inline FixedString NrItems()	{ return "Nr Items";}
    inline FixedString NrValues()	{ return "Nr Values"; }
    inline FixedString Object()		{ return "Object"; }
    inline FixedString Offset()		{ return "Offset"; }
    inline FixedString OffsetRange()	{ return "Offset Range"; }
    inline FixedString Order()		{ return "Order"; }
    inline FixedString Output()		{ return "Output"; }
    inline FixedString Pars()		{ return "Parameters"; }
    inline FixedString PickSet()	{ return "PickSet"; }
    inline FixedString Polygon()	{ return "Polygon"; }
    inline FixedString Position(int n=1) { return n<2 ?"Position":"Positions"; }
    inline FixedString Property()	{ return "Property"; }
    inline FixedString Provider()	{ return "Provider"; }
    inline FixedString Probe()		{ return "Probe"; }
    inline FixedString Python()		{ return "Python"; }
    inline FixedString Quiet()		{ return "quiet"; }
    inline FixedString Random()		{ return "Random"; }
    inline FixedString RandomLine()	{ return "RandomLine"; }
    inline FixedString Range()		{ return "Range"; }
    inline FixedString Region()		{ return "Region"; }
    inline FixedString Sampling()	{ return "Sampling"; }
    inline FixedString Scale()		{ return "Scale"; }
    inline FixedString SeisID()		{ return "SeisID"; }
    inline FixedString SeisCubePositions() { return "Seismic Cube Positions"; }
    inline FixedString Selection()	{ return "Selection"; }
    inline FixedString SelectionStatus(){ return "Selection status"; }
    inline FixedString Series()		{ return "Series"; }
    inline FixedString Server()		{ return "Server"; }
    inline FixedString ServerNm()	{ return "Server_Name"; }
    inline FixedString Setup()		{ return "Setup"; }
    inline FixedString Shortcuts()	{ return "Shortcuts"; }
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
    inline FixedString Target(int n=1)	{ return n<2 ? "Target" : "Targets"; }
    inline FixedString TermEm()		{ return "Terminal Emulator"; }
    inline FixedString Time()		{ return "Time"; }
    inline FixedString TimeSort()	{ return "Time Sort"; }
    inline FixedString Title()		{ return "Title"; }
    inline FixedString TmpStor()	{ return "Temporary storage location"; }
    inline FixedString TODO()		{ return "TODO"; }
    inline FixedString TotalNr()	{ return "Total Nr"; }
    inline FixedString Trace(int n=1)	{ return n<2 ? "Trace" : "Traces"; }
    inline FixedString TraceNr()	{ return "Trace number"; }
    inline FixedString TraceKey()	{ return "Trace key"; }
    inline FixedString TVD(int n=1)	{ return n<2 ? "TVD" : "TVDs"; }
    inline FixedString TVDSS()		{ return "TVDSS"; }
    inline FixedString TWT()		{ return "TWT"; }
    inline FixedString Type(int n=1)	{ return n<2 ? "Type" : "Types"; }
    inline FixedString TwoD()		{ return "2D"; }
    inline FixedString Thickness()	{ return "Thickness"; }
    inline FixedString ThreeD()		{ return "3D"; }
    inline FixedString Undef()		{ return "Undefined"; }
    inline FixedString Unit(int n=1)	{ return n<2 ? "Unit" : "Units"; }
    inline FixedString User()		{ return "User"; }
    inline FixedString Value(int n=1)	{ return n<2 ? "Value" : "Values"; }
    inline FixedString ValueRange()	{ return "Value Range"; }
    inline FixedString Version()	{ return "Version"; }
    inline FixedString Volume()		{ return "Volume"; }
    inline FixedString WaveletID()	{ return "Wavelet ID"; }
    inline FixedString Weight()		{ return "Weight"; }
    inline FixedString Well()		{ return "Well"; }
    inline FixedString X()		{ return "X"; }
    inline FixedString XCoord(int n=1)	{ return n<2 ? "X-Coord" : "X-Coords"; }
    inline FixedString XOffset()	{ return "X Offset"; }
    inline FixedString XRange()		{ return "X range"; }
    inline FixedString Y()		{ return "Y"; }
    inline FixedString Y2()		{ return "Y2"; }
    inline FixedString YCoord(int n=1)	{ return n<2 ? "Y-Coord" : "Y-Coords"; }
    inline FixedString YOffset()	{ return "Y Offset"; }
    inline FixedString YRange()		{ return "Y range"; }
    inline FixedString Yes()		{ return "Yes"; }
    inline FixedString Z()		{ return "Z"; }
    inline FixedString ZDomain()	{ return "Z Domain"; }
    inline FixedString ZCoord()		{ return "Z-Coord"; }
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

};
