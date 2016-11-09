#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Oct 2004
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uisip.h"


mExpClass(uiIo) ui2DSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(ui2DSurvInfoProvider);
public:
			ui2DSurvInfoProvider()
			    : xyft_(false)	{}

    virtual uiString	usrText() const		{ return tr("Set for 2D only");}
    virtual uiDialog*	dialog(uiParent*,TDInfo);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    virtual const char*	iconName() const
					{ return "seismicline2dcollection"; }

    virtual bool	xyInFeet() const	{ return xyft_; }

protected:

    bool		xyft_;
};
