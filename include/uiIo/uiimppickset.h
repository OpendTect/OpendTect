#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.3 2003-11-07 12:21:54 bert Exp $
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
