#ifndef uiodfaulttoolman_h
#define uiodfaulttoolman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Jaap Glas
 Date:		December 2009
 RCS:		$Id: uiodfaulttoolman.h,v 1.1 2010-01-27 13:48:27 cvsjaap Exp $
________________________________________________________________________


-*/


#include "uidialog.h"


class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiLineEdit;
class uiODMain;
class uiSurfaceWrite;
class uiToolBar;

namespace EM			{ class FaultStickSet; }
namespace visSurvey		{ class FaultStickSetDisplay; }


mClass uiFaultStickTransferDlg : public uiDialog
{
public:
				uiFaultStickTransferDlg(uiParent*,
							uiComboBox* tbcombo);
				~uiFaultStickTransferDlg();

    bool			transferToFault() const;
    const uiIOObjSel*		getObjSel() const;
    bool			displayAfterwards() const;

    uiComboBox*			getDlgOutputCombo();

protected:

    void			outputTypeChg(CallBacker*);
    void			outputComboChg(CallBacker*);

    uiGenInput*			outtypefld_;
    uiSurfaceWrite*		faultoutputfld_;
    uiSurfaceWrite*		fssoutputfld_;
    uiComboBox*			tboutputcombo_;
    uiCheckBox*			displayfld_;
};


mClass uiODFaultToolMan : public CallBacker
{
public:
    				uiODFaultToolMan(uiODMain*);
				~uiODFaultToolMan();

    uiToolBar*			getToolBar() const;

protected:
    void			finaliseDoneCB( CallBacker* );

    void			treeItemDeselCB(CallBacker*);
    void			treeItemSelCB(CallBacker*);
    void			addRemoveEMObjCB(CallBacker*);
    void			clearCurDisplayObj();

    void			editSelectToggleCB(CallBacker*);
    void			outputComboChg(CallBacker*);
    void			stickTransferCB(CallBacker*);
    void			popupSettingsCB(CallBacker*);

    uiODMain*			appl_;
    uiToolBar*			toolbar_;
    uiFaultStickTransferDlg*	settingsdlg_;

    int				editselbutidx_;
    int				transferbutidx_;
    int				settingsbutidx_;

    uiComboBox*			tboutputcombo_;

    visSurvey::FaultStickSetDisplay* curfssd_;
    int				curemid_;
};


#endif
