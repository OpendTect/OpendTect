#ifndef uiodfaulttoolman_h
#define uiodfaulttoolman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Jaap Glas
 Date:		December 2009
 RCS:		$Id: uiodfaulttoolman.h,v 1.8 2010-08-17 11:33:49 cvsranojay Exp $
________________________________________________________________________


-*/


#include "multiid.h"
#include "timer.h"
#include "uidialog.h"


class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiLineEdit;
class uiODMain;
class uiSurfaceWrite;
class uiToolBar;
class uiToolButton;
class uiColorInput;


namespace EM			{ class FaultStickSet; }
namespace visSurvey		{ class FaultDisplay;
    				  class FaultStickSetDisplay; }

class uiODFaultToolMan;

mClass uiFaultStickTransferDlg : public uiDialog
{
public:

    mClass Setup
    {
    public:			Setup()
				    : outputfault_( true )
				    , displayifnot_( false )
				    , saveifdisplayed_( true )
				    , sequelnaming_( true )
				    , colorrandom_( false )
				{}

	mDefSetupMemb(bool,outputfault)
	mDefSetupMemb(bool,displayifnot)
	mDefSetupMemb(bool,saveifdisplayed)
	mDefSetupMemb(bool,sequelnaming)
	mDefSetupMemb(bool,colorrandom)
    };

				uiFaultStickTransferDlg(uiODMain&, const Setup&,
							uiODFaultToolMan*);
				~uiFaultStickTransferDlg();

    uiIOObjSel*			getObjSel();
    uiColorInput*		getOutputColor(); 

    bool			displayAfterwards() const;
    bool			saveAfterwards() const;
    bool			generateSequelName() const;
    bool			randomSequelColor() const;

    void			setOutputFields(const uiComboBox& f3dcombo,
						const uiComboBox& fsscombo);

protected:
    void			finaliseDoneCB( CallBacker* );

    void			outputTypeChg(CallBacker*);
    void			outputComboChg(CallBacker*);
    void			outputColorChg(CallBacker*);
    void			displayCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			displayChg(CallBacker*);
    void			sequelNameCB(CallBacker*);

    uiODMain&			appl_;
    uiODFaultToolMan*		ftbman_;	

    uiGenInput*			outtypefld_;
    uiSurfaceWrite*		faultoutputfld_;
    uiSurfaceWrite*		fssoutputfld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			displayfld_;
    uiCheckBox*			savefld_;
    uiCheckBox*			sequelnamefld_;
    uiGenInput*			sequelcolorfld_;

    bool			displayifnot_;
    bool			saveifdisplayed_;
};


mClass uiODFaultToolMan : public CallBacker
{
public:
    				uiODFaultToolMan(uiODMain&);
				~uiODFaultToolMan();

    uiToolBar*			getToolBar();
    uiComboBox*			getOutputCombo(); 
    uiColorInput*		getOutputColor(); 
    bool			isOutputDisplayed() const;

protected:
    void			finaliseDoneCB( CallBacker* );

    void			displayModeChg(CallBacker*);
    void			treeItemSelCB(CallBacker*);
    void			treeItemDeselCB(CallBacker*);
    void			addRemoveEMObjCB(CallBacker*);
    void			deselTimerCB(CallBacker*);
    void			clearCurDisplayObj();
    void			enableToolbar(bool yn);
    void			showSettings(bool yn);

    void			updateToolbarCB(CallBacker*);
    void			editSelectToggleCB(CallBacker*);
    void			outputComboChg(CallBacker*);
    void			colorPressedCB(CallBacker*);
    void			outputColorChg(CallBacker*);
    void			surveyChg(CallBacker*);
    void			undoCB(CallBacker*);
    void			redoCB(CallBacker*);

    void			stickCopyCB(CallBacker*);
    void			stickMoveCB(CallBacker*);
    void			stickRemovalCB(CallBacker*);
    void			transferSticks(bool copy=false);

    void			displayUpdate();
    void			afterTransferUpdate();

    void			settingsToggleCB(CallBacker*);
    void			settingsClosedCB(CallBacker*);

    uiIOObjSel*			getObjSel();
    const uiIOObjSel*		getObjSel() const;

    bool			displayAfterwards() const;
    bool			saveAfterwards() const;

    bool			areSticksAccessible() const;
    void			enableStickAccess(bool yn);

    uiODMain&			appl_;
    uiToolBar*			toolbar_;
    uiFaultStickTransferDlg*	settingsdlg_;

    uiFaultStickTransferDlg::Setup settingssetup_;

    int				editbutidx_;
    int				selbutidx_;
    int				removalbutidx_;
    int				copybutidx_;
    int				movebutidx_;
    int				settingsbutidx_;
    int				undobutidx_;
    int				redobutidx_;

    uiComboBox*			tboutputcombo_;
    uiToolButton*		tbcolorbutton_;
    
    uiSurfaceWrite*		manfaultoutput_;
    uiSurfaceWrite*		manfssoutput_;
    uiColorInput*		manoutputcolor_;

    visSurvey::FaultDisplay*	curfltd_;
    visSurvey::FaultStickSetDisplay* curfssd_;
    int				curemid_;

    bool			tracktbwashidden_;
    bool			selectmode_;

    Timer			deseltimer_;

    Color			newcolor_;
    MultiID			newcolormid_;
};


#endif
