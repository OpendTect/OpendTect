#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.1 2002-05-22 09:25:04 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiIOObjSel;


/*! \brief Dialog for horizon selection */

class uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*);
			~uiImportHorizon();

protected:

    uiIOObjSel*		outfld;
    uiFileInput*	infld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();

    CtxtIOObj&		ctio;
};


#endif
