#ifndef uicreatelogcubedlg_h
#define uicreatelogcubedlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
 RCS:           $Id: uicreatelogcubedlg.h,v 1.3 2012/05/08 07:36:23 cvsbruno Exp $
 _______________________________________________________________________

-*/


#include "uidialog.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiListBox;
class uiMultiWellLogSel;

namespace Well { class Data; class LogSet; }


//4.3 only
mClass uiMultiWellCreateLogCubeDlg : public uiDialog
{
public:
    				uiMultiWellCreateLogCubeDlg(uiParent*);
protected:

    uiMultiWellLogSel*		welllogsel_;
    uiGenInput*			savefld_;
    uiLabeledSpinBox*		repeatfld_;
    
    bool			acceptOK(CallBacker*);
    void			initDlg(CallBacker*);
};


mClass uiCreateLogCubeDlg : public uiDialog
{
public:
    				uiCreateLogCubeDlg(uiParent*,const Well::Data&);

protected:

    const Well::Data& 		wd_;
    uiListBox*			loglistfld_;
    uiGenInput*			savefld_;
    uiLabeledSpinBox*		repeatfld_;
    
    bool			acceptOK(CallBacker*);
};

#endif
