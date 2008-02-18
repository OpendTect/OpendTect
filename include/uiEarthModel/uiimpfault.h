#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.3 2008-02-18 11:00:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiScaler;


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
