#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.11 2009-05-29 11:08:55 cvsbert Exp $
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
class uiSelZRange;
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
    uiSelZRange*	zrgfld_;
    uiGenInput*		tmzrgfld_;
    uiCheckBox*		tmzrginftbox_;
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
    void		haveTrckSel(CallBacker*);

    friend class	uiWellImportAscOptDlg;
};


#endif
