#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2002

 RCS:		$Id: keystrs.h,v 1.48 2009-04-06 07:25:31 cvsnanne Exp $
________________________________________________________________________

-*/
 
 
#include "gendefs.h"

#undef mImpl

#ifdef KEYSTRS_IMPL
# define mImpl(s) = s
#else
# define mImpl(s) /* empty */
#endif

#define mExt mBasicExtern

/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    mExt const char*	Attribute	mImpl("Attribute");
    mExt const char*	Attributes	mImpl("Attributes");
    mExt const char*	Azimuth		mImpl("Azimuth");
    mExt const char*	Color		mImpl("Color");
    mExt const char*	Cube		mImpl("Cube");
    mExt const char*	DataType	mImpl("DataType");
    mExt const char*	Depth		mImpl("Depth");
    mExt const char*	Desc		mImpl("Description");
    mExt const char*	Factor		mImpl("Factor");
    mExt const char*	FileName	mImpl("File name");
    mExt const char*	Filter		mImpl("Filter");
    mExt const char*	FloatUdf	mImpl("1e30");
    mExt const char*	Geometry	mImpl("Geometry");
    mExt const char*	ID		mImpl("ID");
    mExt const char*	IOSelection	mImpl("I/O Selection");
    mExt const char*	Keys		mImpl("Keys");
    mExt const char*	LineKey		mImpl("Line key");
    mExt const char*	Log		mImpl("Log");
    mExt const char*	Name		mImpl("Name");
    mExt const char*	No		mImpl("No");
    mExt const char*	None		mImpl("None");
    mExt const char*	Offset		mImpl("Offset");
    mExt const char*	Output		mImpl("Output");
    mExt const char*	Pars		mImpl("Parameters");
    mExt const char*	Polygon		mImpl("Polygon");
    mExt const char*	Position	mImpl("Position");
    mExt const char*	Random		mImpl("Random");
    mExt const char*	Range		mImpl("Range");
    mExt const char*	Scale		mImpl("Scale");
    mExt const char*	Selection	mImpl("Selection");
    mExt const char*	Subsample	mImpl("Subsample");
    mExt const char*	Shortcuts	mImpl("Shortcuts");
    mExt const char*	Size		mImpl("Size");
    mExt const char*	Steering	mImpl("Steering");
    mExt const char*	StratRef	mImpl("Strat Level");
    mExt const char*	Subsel		mImpl("Subsel");
    mExt const char*	Surface		mImpl("Surface");
    mExt const char*	Survey		mImpl("Survey");
    mExt const char*	Table		mImpl("Table");
    mExt const char*	Target		mImpl("Target");
    mExt const char*	Time		mImpl("Time");
    mExt const char*	Title		mImpl("Title");
    mExt const char*    TraceNr		mImpl("Trace number");
    mExt const char*	Type		mImpl("Type");
    mExt const char*	Undef		mImpl("Undefined");
    mExt const char*	Value		mImpl("Value");
    mExt const char*	Yes		mImpl("Yes");

    mExt const char*	Average		mImpl("Average");
    mExt const char*	Maximum		mImpl("Maximum");
    mExt const char*	Median		mImpl("Median");
    mExt const char*	Minimum		mImpl("Minimum");
    mExt const char*	StdDev		mImpl("StdDev");
    mExt const char*	Sum		mImpl("Sum");

    mExt const char*	BinIDSel	mImpl("BinID selection");
    mExt const char*	InlRange	mImpl("In-line range");
    mExt const char*	FirstInl	mImpl("First In-line");
    mExt const char*	LastInl		mImpl("Last In-line");
    mExt const char*	StepInl		mImpl("Step In-line");
    mExt const char*	StepOutInl	mImpl("Stepout In-line");
    mExt const char*	CrlRange	mImpl("Cross-line range");
    mExt const char*	FirstCrl	mImpl("First Cross-line");
    mExt const char*	LastCrl		mImpl("Last Cross-line");
    mExt const char*	StepCrl		mImpl("Step Cross-line");
    mExt const char*	StepOutCrl	mImpl("Stepout Cross-line");
    mExt const char*	ZRange		mImpl("Z range");
    mExt const char*    FirstTrc	mImpl("First Trace");
    mExt const char*	LastTrc		mImpl("Last Trace");
    mExt const char*    TrcRange        mImpl("Trace Range");

    mExt const char*	TmpStor		mImpl("Temporary storage location");
    mExt const char*	LogFile		mImpl("Log file");
    mExt const char*	Version		mImpl("Version");

	
    mExt const char*	Default		mImpl("Default");
    mExt const char*	DefCube		mImpl("Default.Cube");
    mExt const char*	DefLineSet	mImpl("Default.LineSet");
    mExt const char*	DefLine		mImpl("Default.Line");
    mExt const char*	DefAttribute	mImpl("Default.Attribute");
    mExt const char*	DefPS3D		mImpl("Default.PS3D Data Store");
    mExt const char*	DefPS2D		mImpl("Default.PS2D Data Store");
    mExt const char*	DefWavelet	mImpl("Default.Wavelet");

};

#undef mExt
#undef mImpl

#endif
