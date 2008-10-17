#ifndef ui2dsip_h
#define ui2dsip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Oct 2004
 RCS:           $Id: ui2dsip.h,v 1.3 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisip.h"


class ui2DSurvInfoProvider : public uiSurvInfoProvider
{
public:

    const char*		usrText() const	{ return "Set for 2D only"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

};


#endif
