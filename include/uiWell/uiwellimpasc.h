#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.1 2003-08-26 15:25:52 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiIOObjSel;


/*! \brief Dialog for well import from Ascii */

class uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiIOObjSel*		outfld;
    uiFileInput*	infld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doWork();

    CtxtIOObj&		ctio;
};


#endif
