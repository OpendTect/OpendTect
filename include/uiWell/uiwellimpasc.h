#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.4 2003-10-31 14:26:39 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for well import from Ascii */

class uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiFileInput*	infld;
    uiFileInput*	d2tfld;
    uiGenInput*		unitfld;
    uiCheckBox*		tvdfld;
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
