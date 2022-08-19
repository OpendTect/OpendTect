#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisip.h"
#include "iopar.h"

/* uiSurvInfoProvider taking its source in (a) SEG-Y file(s) */

mExpClass(uiSEGYTools) uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYSurvInfoProvider)
public:

			uiSEGYSurvInfoProvider();
			~uiSEGYSurvInfoProvider();

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    bool		xyInFeet() const	{ return xyinft_; }
    virtual const char*	iconName() const	{ return "segy"; }

    void		fillLogPars(IOPar&) const;
    IOPar*		getImportPars() const;
    void		startImport(uiParent*,const IOPar&);
    const char*		importAskQuestion() const;
    const uiString	importAskUiQuestion() const;

    IOPar		imppars_;
    bool		xyinft_			= false;
    BufferString	userfilename_;

};
