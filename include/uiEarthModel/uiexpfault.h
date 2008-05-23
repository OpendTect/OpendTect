#ifndef uiexpfault_h
#define uiexpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.h,v 1.3 2008-05-23 11:20:16 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for horizon export */

class uiExportFault : public uiDialog
{
public:
			uiExportFault(uiParent*);
			~uiExportFault();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiCheckBox*		stickfld_;
    uiCheckBox*		nodefld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    bool		writeAscii();
};

#endif
