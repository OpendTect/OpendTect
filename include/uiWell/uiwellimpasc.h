#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.8 2008-08-13 09:27:50 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }


/*! \brief Dialog for well import from Ascii */

class uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiFileInput*	wtinfld;

    Table::FormatDesc&  fd;
    uiTableImpDataSel*  dataselfld;
    uiD2TModelGroup*	d2tgrp;

    uiGenInput*		unitfld;
    uiGenInput*		idfld;
    uiGenInput*		coordfld;
    uiGenInput*		elevfld;
    uiGenInput*		operfld;
    uiGenInput*		statefld;
    uiGenInput*		countyfld;
    uiIOObjSel*		outfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doWork();

    CtxtIOObj&		ctio;
};


#endif
