#ifndef uicreatelogcubedlg_h
#define uicreatelogcubedlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
 RCS:           $Id: uicreatelogcubedlg.h,v 1.1 2011-07-04 11:04:36 cvsbruno Exp $
 _______________________________________________________________________

-*/


#include "uidialog.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiListBox;

namespace Well { class Data; class LogSet; }

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
