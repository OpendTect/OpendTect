#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2004
 RCS:           $Id: uisegysip.h,v 1.6 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uisip.h"


mClass uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{
public:

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

};


#endif
