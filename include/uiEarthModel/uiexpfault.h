#ifndef uiexpfault_h
#define uiexpfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.h,v 1.10 2012-08-03 13:00:56 cvskris Exp $
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiCheckBox;
class uiCheckList;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;


/*! \brief Dialog for horizon export */

mClass(uiEarthModel) uiExportFault : public uiDialog
{
public:
			uiExportFault(uiParent*,const char* type);
			~uiExportFault();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiCheckBox*		zbox_;
    uiCheckList*	stickidsfld_;
    uiCheckBox*		linenmfld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    bool		writeAscii();
};

#endif

