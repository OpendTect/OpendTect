#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.5 2003-08-07 14:35:42 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class CtxtIOObj;
class uiBinIDSubSel;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiScaler;



/*! \brief Dialog for horizon selection */

class uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*);
			~uiImportHorizon();

    bool		doDisplay() const;
    MultiID		getSelID() const;

protected:

    uiFileInput*	infld;
    uiGenInput*		xyfld;
    uiScaler*		scalefld;
    uiBinIDSubSel*	subselfld;
    uiIOObjSel*		outfld;
    uiCheckBox*		displayfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();

    CtxtIOObj&		ctio;
};


#endif
