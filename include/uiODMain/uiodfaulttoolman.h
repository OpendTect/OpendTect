#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "bufstringset.h"
#include "emposid.h"
#include "multiid.h"
#include "timer.h"
#include "uidialog.h"


class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiLineEdit;
class uiODMain;
class uiPushButton;
class uiSurfaceWrite;
class uiToolBar;
class uiToolButton;
class uiColorInput;


namespace EM			{ class FaultStickSet; }
namespace visSurvey		{ class FaultDisplay;
    				  class FaultStickSetDisplay; }


mExpClass(uiODMain) uiFaultStickTransferDlg : public uiDialog
{ mODTextTranslationClass(uiFaultStickTransferDlg);
public:

    enum ColorMode		{ Inherit=0, Random, SerialUserDef,
				  Current, ExistsUserDef,
				  SingleUserDef };
    mExpClass(uiODMain) Setup
    { mODTextTranslationClass(Setup);
    public:			Setup()
				    : displayifnot_( true )
				    , saveifdisplayed_( true )
				    , colormode_( Inherit )
				{}

	mDefSetupMemb(bool,displayifnot)
	mDefSetupMemb(bool,saveifdisplayed)
	mDefSetupMemb(ColorMode,colormode)
    };

				uiFaultStickTransferDlg(uiODMain&,const Setup&);

    bool			displayAfterwards() const;
    bool			saveAfterwards() const;
    int				colorMode() const;

    void			setOutputDisplayed(bool);
    void			setColorMode(int);

    Notifier<uiFaultStickTransferDlg> colormodechg;

protected:

    void			displayCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			colorModeChg(CallBacker*);

    uiCheckBox*			displayfld_;
    uiCheckBox*			savefld_;
    uiGenInput*			serialcolormodefld_;
    uiGenInput*			existscolormodefld_;
    uiGenInput*			singlecolormodefld_;

    bool			displayifnot_;
    bool			saveifdisplayed_;
};


mExpClass(uiODMain) uiODFaultToolMan : public CallBacker
{ mODTextTranslationClass(uiODFaultToolMan);
public:
    				uiODFaultToolMan(uiODMain&);
				~uiODFaultToolMan();

    uiToolBar*			getToolBar();

protected:
    void			finalizeDoneCB( CallBacker* );

    void			displayModeChg(CallBacker*);
    void			treeItemSelCB(CallBacker*);
    void			treeItemDeselCB(CallBacker*);
    void			addRemoveEMObjCB(CallBacker*);
    void			addRemoveVisObjCB(CallBacker*);
    void			deselTimerCB(CallBacker*);
    void			surveyChg(CallBacker*);

    void			enableToolbar(bool yn);
    void			clearCurDisplayObj();

    void			updateToolbarCB(CallBacker*);
    void			editSelectToggleCB(CallBacker*);
    void			outputTypeChg(CallBacker*);
    void			outputActionChg(CallBacker*);
    void			stickRemovalCB(CallBacker*);
    void			undoCB(CallBacker*);
    void			redoCB(CallBacker*);
    void			keyPressedCB(CallBacker*);
    void			keyReleasedCB(CallBacker*);

    void			outputEditTextChg(CallBacker*);
    void			outputComboSelChg(CallBacker*);
    void			editReadyTimerCB(CallBacker*);

    void			processOutputName();
    bool			isOutputNameUsed(uiSurfaceWrite* =0) const;
    void			setOutputName(const char*);
    void			setAuxSurfaceWrite(const char*);

    void			selectOutputCB(CallBacker*);
    void			outputSelectedCB(CallBacker*);

    void			colorPressedCB(CallBacker*);
    void			outputColorChg(CallBacker*);
    void			colorModeChg(CallBacker*);
    void			updateColorMode();

    void			settingsToggleCB(CallBacker*);
    void			settingsClosedCB(CallBacker*);
    void			showSettings(bool yn);

    void			transferSticksCB(CallBacker*);
    void			displayUpdate();
    void			afterTransferUpdate();

    bool			isOutputDisplayed(uiSurfaceWrite* =0) const;
    bool			isInSerialMode() const;
    bool			isInCreateMode() const;

    bool			displayAfterwards() const;
    bool			saveAfterwards() const;
    bool			inheritColor() const;
    bool			randomColor() const;
    bool			currentColor() const;

    BufferStringSet&		getOutputItems();
    void			updateOutputItems(bool clearcuritem);
    void			publishOutputItems();

    void			flashOutputName(bool error,
	    					const char* newname=0);
    void			flashOutputTimerCB(CallBacker*);
    void			flashReset();

    bool			areSticksAccessible() const;
    void			enableStickAccess(bool yn);

    uiIOObjSel*			getObjSel();
    const uiIOObjSel*		getObjSel() const;

    uiODMain&			appl_;
    uiToolBar*			toolbar_;
    uiFaultStickTransferDlg*	settingsdlg_;

    uiFaultStickTransferDlg::Setup settingssetup_;

    bool			tracktbwashidden_;
    bool			selectmode_;

    int				editbutidx_;
    int				selbutidx_;
    int				settingsbutidx_;
    int				gobutidx_;
    int				removalbutidx_;
    int				undobutidx_;
    int				redobutidx_;

    uiComboBox*			transfercombo_;
    uiComboBox*			outputtypecombo_;
    uiComboBox*			outputactcombo_;

    uiComboBox*			outputnamecombo_;
    uiPushButton*		outputselbut_;
    uiToolButton*		colorbut_;

    uiSurfaceWrite*		auxfaultwrite_;
    uiSurfaceWrite*		auxfsswrite_;

    visSurvey::FaultDisplay*	curfltd_;
    visSurvey::FaultStickSetDisplay* curfssd_;
    EM::ObjectID		curemid_;

    Timer			deseltimer_;
    Timer			editreadytimer_;
    Timer			flashtimer_;

    BufferString		flashname_;
    OD::Color			flashcolor_;

    OD::Color			colorbutcolor_;
    OD::Color			randomcolor_;
    OD::Color			usercolor_;
    BufferString		usercolorlink_;

    BufferStringSet		singlefaultitems_;
    BufferStringSet		serialfaultitems_;
    BufferStringSet		allfaultitems_;
    BufferStringSet		singlefssitems_;
    BufferStringSet		serialfssitems_;
    BufferStringSet		allfssitems_;

    static uiString		sKeyCopySelection();
    static uiString		sKeyMoveSelection();
    static uiString		sKeyToFault();
    static uiString		sKeyToFaultStickSet();
    static uiString		sKeyCreateSingleNew();
    static uiString		sKeyCreateNewInSeries();
    static uiString		sKeyMergeWithExisting();
    static uiString		sKeyReplaceExisting();
};
