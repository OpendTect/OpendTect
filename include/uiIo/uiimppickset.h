#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.4 2004-06-23 11:18:46 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for pickset selection */

class uiImpExpPickSet : public uiDialog
{
public:
			uiImpExpPickSet(uiParent*,bool);
			~uiImpExpPickSet();

protected:

    uiIOObjSel*		objfld;
    uiGenInput*		xyfld;
    uiFileInput*	filefld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doImport();
    bool		doExport();

    bool		import;
    CtxtIOObj&		ctio;
};


#endif
