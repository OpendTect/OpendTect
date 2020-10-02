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
namespace SEGY { class Scanner; }


/* uiSurvInfoProvider taking it's source in (a) SEG-Y file(s) */

mExpClass(uiSEGYTools) uiSEGYClassicSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYClassicSurvInfoProvider)
public:

			uiSEGYClassicSurvInfoProvider()
			    : xyinft_(false)	{}

    uiString		usrText() const	{ return tr("Classic SEG-Y scanner"); }
    uiDialog*		dialog(uiParent*,TDInfo);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    bool		xyInFeet() const	{ return xyinft_; }
    virtual const char* iconName() const	{ return "segy_classic"; }

    IOPar*		getImportPars() const
			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&);
    uiString		importAskQuestion() const;

    friend class	uiSEGYSIPMgrDlg;
    IOPar		imppars_;
    bool		xyinft_;
    BufferString	userfilename_;

    void		showReport(const SEGY::Scanner&) const;

};
