#ifndef uiexpfault_h
#define uiexpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.h,v 1.7 2009-07-15 13:47:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for horizon export */

mClass uiExportFault : public uiDialog
{
public:
			uiExportFault(uiParent*,const char* type);
			~uiExportFault();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiCheckBox*		zbox_;
    uiCheckBox*		stickfld_;
    uiCheckBox*		nodefld_;
    uiCheckBox*		linenmfld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    bool		writeAscii();
};

#endif
