#ifndef uiodfaulttoolman_h
#define uiodfaulttoolman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Jaap Glas
 Date:		December 2009
 RCS:		$Id: uiodfaulttoolman.h,v 1.3 2010-02-12 10:38:35 cvsjaap Exp $
________________________________________________________________________


-*/


#include "uidialog.h"
#include "multiid.h"
#include "timer.h"


class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiLineEdit;
class uiODMain;
class uiSurfaceWrite;
class uiToolBar;
class uiColorInput;


namespace EM			{ class FaultStickSet; }
namespace visSurvey		{ class FaultDisplay;
    				  class FaultStickSetDisplay; }

class uiODFaultToolMan;

mClass uiFaultStickTransferDlg : public uiDialog
{
public:
				uiFaultStickTransferDlg(uiODMain&,
							uiODFaultToolMan*);
				~uiFaultStickTransferDlg();

    uiIOObjSel*			getObjSel();
    bool			transferToFault() const;
    bool			startDisplayingAfterwards() const;

    uiComboBox*			getOutputCombo();
    uiColorInput*		getOutputColor(); 

    bool			afterTransferUpdate();

protected:
    void			finaliseDoneCB( CallBacker* );

    void			outputTypeChg(CallBacker*);
    void			outputComboChg(CallBacker*);
    void			outputColorChg(CallBacker*);
    void			displayChg(CallBacker*);
    void			sequelNameCB(CallBacker*);

    uiODMain&			appl_;
    uiODFaultToolMan*		ftbman_;	

    uiGenInput*			outtypefld_;
    uiSurfaceWrite*		faultoutputfld_;
    uiSurfaceWrite*		fssoutputfld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			displayfld_;
    uiCheckBox*			sequelnamefld_;
    uiGenInput*			sequelcolorfld_;

    Color			newcolor_;
    MultiID			newcolormid_;
};


mClass uiODFaultToolMan : public CallBacker
{
public:
    				uiODFaultToolMan(uiODMain&);
				~uiODFaultToolMan();

    uiToolBar*			getToolBar();
    uiComboBox*			getOutputCombo(); 
    uiColorInput*		getOutputColor(); 

protected:
    void			finaliseDoneCB( CallBacker* );

    void			treeItemSelCB(CallBacker*);
    void			treeItemDeselCB(CallBacker*);
    void			addRemoveEMObjCB(CallBacker*);
    void			deselTimerCB(CallBacker*);
    void			clearCurDisplayObj();
    void			enableToolbar(bool yn);
    void			showSettings(bool yn);

    void			editSelectToggleCB(CallBacker*);
    void			outputComboChg(CallBacker*);
    void			outputColorChg(CallBacker*);

    void			stickCopyCB(CallBacker*);
    void			stickMoveCB(CallBacker*);
    void			stickRemovalCB(CallBacker*);
    void			transferSticks(bool copy=false);

    void			settingsToggleCB(CallBacker*);
    void			settingsClosedCB(CallBacker*);

    uiODMain&			appl_;
    uiToolBar*			toolbar_;
    uiFaultStickTransferDlg*	settingsdlg_;

    int				editselbutidx_;
    int				removalbutidx_;
    int				copybutidx_;
    int				movebutidx_;
    int				settingsbutidx_;

    uiComboBox*			tboutputcombo_;
    uiColorInput*		tboutputcolor_;

    visSurvey::FaultDisplay*	curfltd_;
    visSurvey::FaultStickSetDisplay* curfssd_;
    int				curemid_;

    bool			tracktbwashidden_;

    Timer			deseltimer_;
};


#endif
