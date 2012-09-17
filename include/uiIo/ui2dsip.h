#ifndef ui2dsip_h
#define ui2dsip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Oct 2004
 RCS:           $Id: ui2dsip.h,v 1.6 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisip.h"


mClass ui2DSurvInfoProvider : public uiSurvInfoProvider
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
