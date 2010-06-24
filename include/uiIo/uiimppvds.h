#ifndef uiimppvds_h
#define uiimppvds_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2010
 RCS:           $Id: uiimppvds.h,v 1.1 2010-06-24 15:16:51 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class uiFileInput;


mClass uiImpPVDS : public uiDialog 
{
public:
			uiImpPVDS(uiParent*);

protected:

    uiFileInput*	inpfld_;
    uiGenInput*		haveposfld_;
    uiGenInput*		posiscoordfld_;
    uiCheckBox*		havezbox_;
    uiIOObjSel*		outfld_;
    uiGroup*		posgrp_;

    void		havePosSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
