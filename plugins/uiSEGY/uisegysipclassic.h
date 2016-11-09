#ifndef uisegysipclassic_h
#define uisegysipclassic_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisip.h"
#include "iopar.h"
namespace SEGY { class Scanner; }


/* uiSurvInfoProvider using SEG-Y file(s), based on classic wizard */

mExpClass(uiSEGY) uiSEGYClassicSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYClassicSurvInfoProvider)
public:

			uiSEGYClassicSurvInfoProvider()
			    : xyinft_(false)	{}

    const char*		usrText() const	{ return "Classic SEG-Y scanner"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    bool		xyInFeet() const	{ return xyinft_; }
    virtual const char* iconName() const	{ return "segy_classic"; }

    IOPar*		getImportPars() const
			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&);
    const char*		importAskQuestion() const;
    const uiString	importAskUiQuestion() const;

    friend class	uiSEGYSIPMgrDlg;
    IOPar		imppars_;
    bool		xyinft_;

    void		showReport(const SEGY::Scanner&) const;

};


#endif

