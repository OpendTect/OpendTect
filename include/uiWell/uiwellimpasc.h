#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.10 2009-04-20 13:29:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "welldata.h"

class CtxtIOObj;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }


/*! \brief Dialog for well import from Ascii */

mClass uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiFileInput*	wtinfld;
    Well::Data		wd_;
    CtxtIOObj&		ctio;

    Table::FormatDesc&  fd;
    uiTableImpDataSel*  dataselfld;
    uiD2TModelGroup*	d2tgrp;
    uiIOObjSel*		outfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doWork();
    void		doAdvOpt(CallBacker*);

    friend class	uiWellImportAscOptDlg;
};


#endif
