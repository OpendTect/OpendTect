#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 (C)		dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
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
    inline FixedString Attributes()	{ return "Attributes"; }
    inline FixedString Azimuth()	{ return "Azimuth"; }
    inline FixedString Binary()		{ return "Binary"; }
    inline FixedString Body()		{ return "Body"; }
    inline FixedString Color()		{ return "Color"; }
    inline FixedString Component()	{ return "Component"; }
    inline FixedString Content()	{ return "Content"; }
    inline FixedString Crossline()	{ return "Cross-line"; }
    inline FixedString Cube()		{ return "Cube"; }
    inline FixedString DataRoot()	{ return "Data Root"; }
    inline FixedString DataStorage()	{ return "Data storage"; }
    inline FixedString DataType()	{ return "DataType"; }
    inline FixedString Date()		{ return "Date"; }
    inline FixedString Default()	{ return "Default"; }
    inline FixedString Depth()		{ return "Depth"; }
    inline FixedString Desc()		{ return "Description"; }
    inline FixedString EmptyString()	{ return ""; }
    inline FixedString Factor()		{ return "Factor"; }
    inline FixedString Fault()		{ return "Fault"; }
    inline FixedString FileName()	{ return "File name"; }
    inline FixedString Filter()		{ return "Filter"; }
    inline FixedString FloatUdf()	{ return "1e30"; }
    inline FixedString Geometry()	{ return "Geometry"; }
    inline FixedString GeomID()		{ return "GeomID"; }
    inline FixedString Horizon()	{ return "Horizon"; }
    inline FixedString ID()		{ return "ID"; }
    inline FixedString IOSelection()	{ return "I/O Selection"; }
    inline FixedString Inline()		{ return "In-line"; }
    inline FixedString Keys()		{ return "Keys"; }
    inline FixedString Level()		{ return "Level"; }
    inline FixedString Line()		{ return "Line"; }
    inline FixedString LineKey()	{ return "Line key"; }
    inline FixedString LineName()	{ return "Line name"; }
    inline FixedString LineNames()	{ return "Line names"; }
    inline FixedString Log()		{ return "Log"; }
    inline FixedString LogFile()	{ return "Log file"; }
    inline FixedString Name()		{ return "Name"; }
    inline FixedString NewLine()	{ return "\n"; }
    inline FixedString No()		{ return "No"; }
    inline FixedString None()		{ return "None"; }
    inline FixedString Offset()		{ return "Offset"; }
    inline FixedString Output()		{ return "Output"; }
    inline FixedString Pars()		{ return "Parameters"; }
    inline FixedString Polygon()	{ return "Polygon"; }
    inline FixedString Position()	{ return "Position"; }
    inline FixedString Property()	{ return "Property"; }
    inline FixedString Quiet()		{ return "quiet"; }
    inline FixedString Random()		{ return "Random"; }
    inline FixedString Range()		{ return "Range"; }
    inline FixedString Sampling()	{ return "Sampling"; }
    inline FixedString Scale()		{ return "Scale"; }
    inline FixedString Selection()	{ return "Selection"; }
    inline FixedString Subsample()	{ return "Subsample"; }
    inline FixedString Shortcuts()	{ return "Shortcuts"; }
    inline FixedString Size()		{ return "Size"; }
    inline FixedString SpaceString()	{ return " "; }
    inline FixedString Steering()	{ return "Steering"; }
    inline FixedString Stored()		{ return "Stored"; }
    inline FixedString StratRef()	{ return "Strat Level"; }
    inline FixedString Subsel()		{ return "Subsel"; }
    inline FixedString Surface()	{ return "Surface"; }
    inline FixedString Survey()		{ return "Survey"; }
    inline FixedString Table()		{ return "Table"; }
    inline FixedString Target()		{ return "Target"; }
    inline FixedString Time()		{ return "Time"; }
    inline FixedString Title()		{ return "Title"; }
    inline FixedString TmpStor()	{ return "Temporary storage location"; }
    inline FixedString TraceNr()	{ return "Trace number"; }
    inline FixedString Type()		{ return "Type"; }
    inline FixedString TwoD()		{ return "2D"; }
    inline FixedString Thickness()	{ return "Thickness"; }
    inline FixedString ThreeD()		{ return "3D"; }
    inline FixedString Undef()		{ return "Undefined"; }
    inline FixedString Unit()		{ return "Unit"; }
    inline FixedString User()		{ return "User"; }
    inline FixedString Value()		{ return "Value"; }
    inline FixedString Version()	{ return "Version"; }
    inline FixedString Well()		{ return "Well"; }
    inline FixedString XCoord()		{ return "X-Coord"; }
    inline FixedString YCoord()		{ return "Y-Coord"; }
    inline FixedString Yes()		{ return "Yes"; }
    inline FixedString ZRange()		{ return "Z range"; }

    // Stats
    inline FixedString Average()	{ return "Average"; }
    inline FixedString Maximum()	{ return "Maximum"; }
    inline FixedString Median()		{ return "Median"; }
    inline FixedString Minimum()	{ return "Minimum"; }
    inline FixedString StdDev()		{ return "StdDev"; }
    inline FixedString Sum()		{ return "Sum"; }
    inline FixedString Variance()	{ return "Variance"; }

    // (Horizontal) position selection in the survey
    inline FixedString InlRange()	{ return "In-line range"; }
    inline FixedString FirstInl()	{ return "First In-line"; }
    inline FixedString LastInl()	{ return "Last In-line"; }
    inline FixedString StepInl()	{ return "Step In-line"; }
    inline FixedString StepOutInl()	{ return "Stepout In-line"; }
    inline FixedString CrlRange()	{ return "Cross-line range"; }
    inline FixedString FirstCrl()	{ return "First Cross-line"; }
    inline FixedString LastCrl()	{ return "Last Cross-line"; }
    inline FixedString StepCrl()	{ return "Step Cross-line"; }
    inline FixedString StepOutCrl()	{ return "Stepout Cross-line"; }
    inline FixedString FirstTrc()	{ return "First Trace"; }
    inline FixedString LastTrc()	{ return "Last Trace"; }
    inline FixedString TrcRange()	{ return "Trace Range"; }

};


// Makes compat with 4.4 easier
#define mGetKeyStr(id)	sKey::id()


#endif
