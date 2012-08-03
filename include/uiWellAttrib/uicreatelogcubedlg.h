#ifndef uicreatelogcubedlg_h
#define uicreatelogcubedlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
 RCS:           $Id: uicreatelogcubedlg.h,v 1.3 2012-08-03 13:01:21 cvskris Exp $
 _______________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uidialog.h"

class MultiID;
class uiGenInput;
class uiLabeledSpinBox;
class uiMultiWellLogSel;

mClass(uiWellAttrib) uiCreateLogCubeDlg : public uiDialog
{
public:
    				uiCreateLogCubeDlg(uiParent*,const MultiID*);

protected:

    uiGenInput*			savefld_;
    uiLabeledSpinBox*		repeatfld_;
    uiMultiWellLogSel*		welllogsel_;
    
    bool			acceptOK(CallBacker*);
};

#endif

