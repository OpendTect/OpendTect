#ifndef uiwelllogetool_h
#define uiwelllogetool_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uiwelllogtools.h,v 1.1 2011-01-21 16:02:36 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiMultiWellLogSel;
class uiPushButton;
class uiWellLogDisplay;


namespace Well { class Data; class Log; }


mClass uiWellLogToolWin : public uiMainWin
{
public:	
				uiWellLogToolWin(uiParent*,
						ObjectSet<Well::Log>&);
				~uiWellLogToolWin();

protected:

    uiComboBox*			actionfld_;
    uiCheckBox*			overwritefld_;
    uiGenInput*			savefld_;
    uiPushButton*		applybut_;
    uiPushButton*               okbut_;
    uiPushButton*               cancelbut_;

    ObjectSet<Well::Log>	logs_;
    ObjectSet<Well::Log>	outplogs_;
    ObjectSet<uiWellLogDisplay> logdisps_;

    void			overWriteCB(CallBacker*);
    void			applyPushedCB(CallBacker*);
    void			displayOutpLogs(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
};


mClass uiWellLogToolWinMgr : public uiDialog
{
public:
			uiWellLogToolWinMgr(uiParent*);
protected:

    uiMultiWellLogSel*	welllogselfld_;
    bool		acceptOK(CallBacker*);
};



#endif
