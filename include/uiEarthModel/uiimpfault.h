#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.1 2002-09-23 07:10:05 kristofer Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiScaler;
class uiBinIDSubSel;


/*! \brief Dialog for fault selection */

class uiImportLMKFault : public uiDialog
{
public:
			uiImportLMKFault(uiParent*);
			~uiImportLMKFault();

protected:

    uiFileInput*	infld;
    uiFileInput*	formatfilefld;
    uiIOObjSel*		outfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();

    CtxtIOObj&		ctio;
};


#endif
