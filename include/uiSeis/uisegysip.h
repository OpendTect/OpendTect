#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Feb 2004
 RCS:           $Id: uisegysip.h,v 1.4 2008-10-08 15:57:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
class MultiID;


class uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{
public:

			uiSEGYSurvInfoProvider(MultiID&);

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

    MultiID&		segyid;

};


#endif
