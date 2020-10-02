#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2004
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisip.h"
#include "iopar.h"

/* uiSurvInfoProvider taking it's source in (a) SEG-Y file(s) */

mExpClass(uiSEGYTools) uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYSurvInfoProvider)
public:

			uiSEGYSurvInfoProvider()
			    : xyinft_(false)
			    , tdinfo_(uiSurvInfoProvider::Time)
			    , tdinfoknown_(false)
			{}

    uiString		usrText() const	{ return tr("Scan SEG-Y file(s)"); }
    uiDialog*		dialog(uiParent*,TDInfo);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    TDInfo		tdInfo(bool&) const;
    bool		xyInFeet() const	{ return xyinft_; }
    virtual const char*	iconName() const	{ return "segy"; }

    IOPar*		getImportPars() const
			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&);
    uiString		importAskQuestion() const;

    IOPar		imppars_;
    TDInfo		tdinfo_;
    bool		xyinft_;
    bool		tdinfoknown_;
    BufferString	userfilename_;

};
