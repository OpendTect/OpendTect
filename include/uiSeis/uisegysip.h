#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Feb 2004
 RCS:           $Id: uisegysip.h,v 1.2 2004-03-01 13:27:06 bert Exp $
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
    bool		getInfo(uiDialog*,BinIDSampler&,StepInterval<double>&,
	    			Coord crd[3]);
    const char*		scanFile() const;

    MultiID&		segyid;

};


#endif
