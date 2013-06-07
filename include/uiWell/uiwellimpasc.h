#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "welldata.h"

class CtxtIOObj;
class uiLabel;
class uiCheckBox;
class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class uiD2TModelGroup;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }


/*! \brief Dialog for well import from Ascii */

mClass uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiFileInput*	trckinpfld_;
    uiCheckBox*		havetrckbox_;
    uiGenInput*		coordfld_;
    uiGenInput*		kbelevfld_;
    uiGenInput*		tdfld_;
    uiLabel*		vertwelllbl_;

    Well::Data		wd_;
    CtxtIOObj&		ctio_;

    Table::FormatDesc&  fd_;
    uiTableImpDataSel*  dataselfld_;
    uiD2TModelGroup*	d2tgrp_;
    uiIOObjSel*		outfld_;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doWork();
    void		doAdvOpt(CallBacker*);
    void		trckFmtChg(CallBacker*);
    void		haveTrckSel(CallBacker*);

    friend class	uiWellImportAscOptDlg;
};


#endif
