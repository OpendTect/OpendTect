#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.2 2003-11-07 12:21:54 bert Exp $
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
