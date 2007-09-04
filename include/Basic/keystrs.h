#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2002
 RCS:		$Id: keystrs.h,v 1.21 2007-09-04 08:07:49 cvshelene Exp $
________________________________________________________________________

-*/
 
 
#include <gendefs.h>

#undef mImpl

#ifdef KEYSTRS_IMPL
# define mImpl(s) = s
#else
# define mImpl(s) /* empty */
#endif

/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    extern const char*	Attribute	mImpl("Attribute");
    extern const char*	Attributes	mImpl("Attributes");
    extern const char*	Color		mImpl("Color");
    extern const char*	Time		mImpl("Time");
    extern const char*	TimeDomain	mImpl("Time Domain");
    extern const char*	Depth		mImpl("Depth");
    extern const char*	DepthDomain	mImpl("Depth Domain");
    extern const char*	Desc		mImpl("Description");
    extern const char*	Factor		mImpl("Factor");
    extern const char*	FileName	mImpl("File name");
    extern const char*	FloatUdf	mImpl("1e30");
    extern const char*	Geometry	mImpl("Geometry");
    extern const char*	IOSelection	mImpl("I/O Selection");
    extern const char*	Keys		mImpl("Keys");
    extern const char*	LineKey		mImpl("Line key");
    extern const char*	Name		mImpl("Name");
    extern const char*	No		mImpl("No");
    extern const char*	None		mImpl("None");
    extern const char*	Pars		mImpl("Parameters");
    extern const char*	PSSeis		mImpl("Pre-Stack Seismics");
    extern const char*	Range		mImpl("Range");
    extern const char*	Shortcuts	mImpl("Shortcuts");
    extern const char*	Size		mImpl("Size");
    extern const char*	Steering	mImpl("Steering");
    extern const char*	StratRef	mImpl("Strat Level");
    extern const char*	Survey		mImpl("Survey");
    extern const char*	Table		mImpl("Table");
    extern const char*	Type		mImpl("Type");
    extern const char*	Title		mImpl("Title");
    extern const char*	Undef		mImpl("Undefined");
    extern const char*	Value		mImpl("Value");
    extern const char*	Wheeler		mImpl("Wheeler");
    extern const char*	Yes		mImpl("Yes");

    extern const char*	Average		mImpl("Average");
    extern const char*	Maximum		mImpl("Maximum");
    extern const char*	Median		mImpl("Median");
    extern const char*	Minimum		mImpl("Minimum");
    extern const char*	StdDev		mImpl("StdDev");
    extern const char*	Sum		mImpl("Sum");

    extern const char*	BinIDSel	mImpl("BinID selection");
    extern const char*	InlRange	mImpl("In-line range");
    extern const char*	FirstInl	mImpl("First In-line");
    extern const char*	LastInl		mImpl("Last In-line");
    extern const char*	StepInl		mImpl("Step In-line");
    extern const char*	StepOutInl	mImpl("Stepout In-line");
    extern const char*	CrlRange	mImpl("Cross-line range");
    extern const char*	FirstCrl	mImpl("First Cross-line");
    extern const char*	LastCrl		mImpl("Last Cross-line");
    extern const char*	StepCrl		mImpl("Step Cross-line");
    extern const char*	StepOutCrl	mImpl("Stepout Cross-line");
    extern const char*	ZRange		mImpl("Z range");

    extern const char*	TmpStor		mImpl("Temporary storage location");
    extern const char*	LogFile		mImpl("Log file");
};


#undef mImpl

#endif
