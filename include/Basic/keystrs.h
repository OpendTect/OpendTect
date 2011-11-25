#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002

 RCS:		$Id: keystrs.h,v 1.66 2011-11-25 17:24:16 cvsyuancheng Exp $
________________________________________________________________________

-*/
 
 
#include "gendefs.h"
#include "fixedstring.h"

#undef mImpl


#ifdef KEYSTRS_IMPL
# define mImpl(s) = s
#ifdef __msvc__
#define mExt mBasicExtern
#else 
# define mExt
#endif
#else
# define mImpl(s) /* empty */
# define mExt mBasicExtern
#endif



/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    mExt FixedString	Ascii		mImpl("Ascii");
    mExt FixedString	Attribute 	mImpl("Attribute");
    mExt FixedString	Attributes	mImpl("Attributes");
    mExt FixedString	Azimuth		mImpl("Azimuth");
    mExt FixedString	Binary		mImpl("Binary");
    mExt FixedString	Body		mImpl("Body");
    mExt FixedString	Color		mImpl("Color");
    mExt FixedString	Component	mImpl("Component");
    mExt FixedString	Crossline	mImpl("Cross-line");
    mExt FixedString	Cube		mImpl("Cube");
    mExt FixedString	DataRoot	mImpl("Data Root");
    mExt FixedString	DataStorage	mImpl("Data storage");
    mExt FixedString	DataType	mImpl("DataType");
    mExt FixedString	Date		mImpl("Date");
    mExt FixedString	Depth		mImpl("Depth");
    mExt FixedString	Desc		mImpl("Description");
    mExt FixedString	EmptyString	mImpl("");
    mExt FixedString	Factor		mImpl("Factor");
    mExt FixedString	FileName	mImpl("File name");
    mExt FixedString	Filter		mImpl("Filter");
    mExt FixedString	FloatUdf	mImpl("1e30");
    mExt FixedString	Geometry	mImpl("Geometry");
    mExt FixedString	ID		mImpl("ID");
    mExt FixedString	IOSelection	mImpl("I/O Selection");
    mExt FixedString	Inline		mImpl("In-line");
    mExt FixedString	Keys		mImpl("Keys");
    mExt FixedString	LineKey		mImpl("Line key");
    mExt FixedString	LineName	mImpl("Line name");
    mExt FixedString	Log		mImpl("Log");
    mExt FixedString	LogFile		mImpl("Log file");
    mExt FixedString	Name		mImpl("Name");
    mExt FixedString	No		mImpl("No");
    mExt FixedString	None		mImpl("None");
    mExt FixedString	Offset		mImpl("Offset");
    mExt FixedString	Output		mImpl("Output");
    mExt FixedString	Pars		mImpl("Parameters");
    mExt FixedString	Polygon		mImpl("Polygon");
    mExt FixedString	Position	mImpl("Position");
    mExt FixedString	Property	mImpl("Property");
    mExt FixedString	Random		mImpl("Random");
    mExt FixedString	Range		mImpl("Range");
    mExt FixedString	Sampling	mImpl("Sampling");
    mExt FixedString	Scale		mImpl("Scale");
    mExt FixedString	Selection	mImpl("Selection");
    mExt FixedString	Subsample	mImpl("Subsample");
    mExt FixedString	Shortcuts	mImpl("Shortcuts");
    mExt FixedString	Size		mImpl("Size");
    mExt FixedString	Steering	mImpl("Steering");
    mExt FixedString	Stored		mImpl("Stored");
    mExt FixedString	StratRef	mImpl("Strat Level");
    mExt FixedString	Subsel		mImpl("Subsel");
    mExt FixedString	Surface		mImpl("Surface");
    mExt FixedString	Survey		mImpl("Survey");
    mExt FixedString	Table		mImpl("Table");
    mExt FixedString	Target		mImpl("Target");
    mExt FixedString	Time		mImpl("Time");
    mExt FixedString	Title		mImpl("Title");
    mExt FixedString    TraceNr		mImpl("Trace number");
    mExt FixedString	Type		mImpl("Type");
    mExt FixedString	Undef		mImpl("Undefined");
    mExt FixedString	Unit		mImpl("Unit");
    mExt FixedString	Value		mImpl("Value");
    mExt FixedString	Version		mImpl("Version");
    mExt FixedString	XCoord		mImpl("X-Coord");
    mExt FixedString	YCoord		mImpl("Y-Coord");
    mExt FixedString	Yes		mImpl("Yes");

    mExt FixedString	Average		mImpl("Average");
    mExt FixedString	Maximum		mImpl("Maximum");
    mExt FixedString	Median		mImpl("Median");
    mExt FixedString	Minimum		mImpl("Minimum");
    mExt FixedString	StdDev		mImpl("StdDev");
    mExt FixedString	Sum		mImpl("Sum");

    mExt FixedString	BinIDSel	mImpl("BinID selection");
    mExt FixedString	InlRange	mImpl("In-line range");
    mExt FixedString	FirstInl	mImpl("First In-line");
    mExt FixedString	LastInl		mImpl("Last In-line");
    mExt FixedString	StepInl		mImpl("Step In-line");
    mExt FixedString	StepOutInl	mImpl("Stepout In-line");
    mExt FixedString	CrlRange	mImpl("Cross-line range");
    mExt FixedString	FirstCrl	mImpl("First Cross-line");
    mExt FixedString	LastCrl		mImpl("Last Cross-line");
    mExt FixedString	StepCrl		mImpl("Step Cross-line");
    mExt FixedString	StepOutCrl	mImpl("Stepout Cross-line");
    mExt FixedString	ZRange		mImpl("Z range");
    mExt FixedString    FirstTrc	mImpl("First Trace");
    mExt FixedString	LastTrc		mImpl("Last Trace");
    mExt FixedString    TrcRange        mImpl("Trace Range");

    mExt FixedString	TmpStor		mImpl("Temporary storage location");

    mExt FixedString	Default		mImpl("Default");
    mExt FixedString	DefCube		mImpl("Default.Cube");
    mExt FixedString	DefLineSet	mImpl("Default.LineSet");
    mExt FixedString	DefLine		mImpl("Default.Line");
    mExt FixedString	DefAttribute	mImpl("Default.Attribute");
    mExt FixedString	DefPS3D		mImpl("Default.PS3D Data Store");
    mExt FixedString	DefPS2D		mImpl("Default.PS2D Data Store");
    mExt FixedString	DefWavelet	mImpl("Default.Wavelet");

};

#undef mExt
#undef mImpl

#endif
