#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.2 2003-07-24 11:57:02 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for pickset selection */

class uiImportPickSet : public uiDialog
{
public:
			uiImportPickSet(uiParent*);
			~uiImportPickSet();

protected:

    uiIOObjSel*		outfld;
    uiGenInput*		xyfld;
    uiFileInput*	infld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();

    CtxtIOObj&		ctio;
};


#endif
