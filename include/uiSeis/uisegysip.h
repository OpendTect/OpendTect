#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2004
 RCS:           $Id: uisegysip.h,v 1.7 2009-01-13 13:52:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisip.h"
#include "iopar.h"


mClass uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{
public:

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,CubeSampling&,Coord crd[3]);

    IOPar*		getImportPars() const
    			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&);

    friend class	uiSEGYSIPMgrDlg;
    IOPar		imppars_;

};


#endif
