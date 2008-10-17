#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2004
 RCS:           $Id: uisegysip.h,v 1.5 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisip.h"


class uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{
public:

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

};


#endif
