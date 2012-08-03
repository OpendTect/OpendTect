#ifndef ui2dsip_h
#define ui2dsip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Oct 2004
 RCS:           $Id: ui2dsip.h,v 1.7 2012-08-03 13:00:58 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uisip.h"


mClass(uiIo) ui2DSurvInfoProvider : public uiSurvInfoProvider
{
public:
    			ui2DSurvInfoProvider()
			    : xyft_(false)	{}

    const char*		usrText() const		{ return "Set for 2D only"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

    bool		xyInFeet() const	{ return xyft_; }

    bool		xyft_;
};


#endif

