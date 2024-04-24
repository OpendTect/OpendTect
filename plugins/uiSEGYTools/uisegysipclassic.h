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
namespace SEGY { class Scanner; }


/* uiSurvInfoProvider using SEG-Y file(s), based on classic wizard */

mExpClass(uiSEGYTools) uiSEGYClassicSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYClassicSurvInfoProvider)
public:

			uiSEGYClassicSurvInfoProvider()
			    : xyinft_(false)	{}

    const char*		usrText() const override
			{ return "Classic SEG-Y scanner"; }

    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,
				TrcKeyZSampling&,Coord crd[3]) override;
    bool		xyInFeet() const override	{ return xyinft_; }
    const char*		iconName() const override
			{ return "segy_classic"; }

    IOPar*		getImportPars() const override
			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&) override;
    uiString		importAskQuestion() const override;
    const uiString	importAskUiQuestion() const;

    friend class	uiSEGYSIPMgrDlg;
    IOPar		imppars_;
    bool		xyinft_;

    void		showReport(const SEGY::Scanner&) const;

};
