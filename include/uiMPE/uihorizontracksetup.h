#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "color.h"
#include "draw.h"
#include "valseriesevent.h"
#include "emseedpicker.h"

#include "uimpe.h"

class uiButtonGroup;
class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiSeisSel;
class uiSlider;
class uiTabStack;
class uiToolBar;
class uiToolButton;


namespace MPE
{

class HorizonAdjuster;
class HorizonTrackerMgr;
class SectionTracker;
class uiCorrelationGroup;
class uiEventGroup;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiHorizonSetupGroup : public uiSetupGroup
{ mODTextTranslationClass(uiHorizonSetupGroup)
public:
				uiHorizonSetupGroup(uiParent*,const char*);
				~uiHorizonSetupGroup();

    void			setSectionTracker(SectionTracker*) override;

    void			setMode(EMSeedPicker::TrackMode) override;
    EMSeedPicker::TrackMode	getMode() const override;
    void			setTrackingMethod(
					EventTracker::CompareMethod) override;
    EventTracker::CompareMethod getTrackingMethod() const override;
    void			setSeedPos(const TrcKeyValue&) override;
    void			setColor(const OD::Color&) override;
    const OD::Color&		getColor() override;
    int				getLineWidth() const override;
    void			setLineWidth(int) override;
    void			setMarkerStyle(const MarkerStyle3D&) override;
    const MarkerStyle3D&	getMarkerStyle() override;

    NotifierAccess*		modeChangeNotifier() override
				{ return &modeChanged_; }
    NotifierAccess*		propertyChangeNotifier() override
				{ return &propertyChanged_; }
    NotifierAccess*		eventChangeNotifier() override;
    NotifierAccess*		correlationChangeNotifier() override;
    NotifierAccess*		varianceChangeNotifier()
				{ return &varianceChanged_; }

    bool			commitToTracker() const override
				{ bool b; return commitToTracker(b); }
    bool			commitToTracker(
					bool& fieldchange) const override;

    void			enableTracking(bool);
    void			showGroupOnTop(const char* grpnm) override;

    void			setMPEPartServer(uiMPEPartServer*) override;

protected:

    virtual void		initStuff();

// General
    uiTabStack*			tabgrp_;
    uiToolBar*			toolbar_;
    int				trackbutid_;
    int				startbutid_;
    int				stopbutid_;
    int				savebutid_;
    int				retrackbutid_;
    int				undobutid_;
    int				redobutid_;

    void			initToolBar();
    void			updateButtonSensitivity();
    void			mpeActionCB(CallBacker*);
    void			enabTrackCB(CallBacker*);
    void			startCB(CallBacker*);
    void			stopCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			retrackCB(CallBacker*);
    void			undoCB(CallBacker*);
    void			redoCB(CallBacker*);
    void			horizonSelCB(CallBacker*);
    void			trackingFinishedCB(CallBacker*);

// Mode Group
    uiGroup*			createModeGroup();
    void			initModeGroup();
    void			seedModeChange(CallBacker*);

    uiButtonGroup*		modeselgrp_;
    uiCheckBox*			betweenseedsfld_;
    uiCheckBox*			snapfld_;
    uiGenInput*			methodfld_;
    uiGenInput*			failfld_			= nullptr;

// Event and Correlation Group
    uiEventGroup*		eventgrp_;
    uiCorrelationGroup*		correlationgrp_;

// Variance Group
    uiGroup*			createVarianceGroup();
    void			initVarianceGroup();
    void			selUseVariance(CallBacker*);
    void			varianceChangeCB(CallBacker*);

    uiGenInput*			usevarfld_;
    uiSeisSel*			variancefld_;
    uiGenInput*			varthresholdfld_;


// Property Group
    uiGroup*			createPropertyGroup();
    void			initPropertyGroup();
    void			colorChangeCB(CallBacker*);
    void			specColorChangeCB(CallBacker*);
    void			seedTypeSel(CallBacker*);
    void			seedColSel(CallBacker*);
    void			seedSliderMove(CallBacker*);

    uiColorInput*		colorfld_;
    uiSlider*			linewidthfld_;
    uiGenInput*			seedtypefld_;
    uiColorInput*		seedcolselfld_;
    uiSlider*			seedsliderfld_;
    uiColorInput*		parentcolfld_;
    uiColorInput*		selectioncolfld_;
    uiColorInput*		lockcolfld_;


    bool			is2d_;
    EMSeedPicker::TrackMode	mode_;
    MarkerStyle3D		markerstyle_;

    HorizonTrackerMgr*		trackmgr_			= nullptr;
    SectionTracker*		sectiontracker_			= nullptr;
    HorizonAdjuster*		horadj_				= nullptr;

    Notifier<uiHorizonSetupGroup> modeChanged_;
    Notifier<uiHorizonSetupGroup> varianceChanged_;
    Notifier<uiHorizonSetupGroup> propertyChanged_;

    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();

    uiMPEPartServer*		mps_				= nullptr;
};


mExpClass(uiMPE) uiBaseHorizonSetupGroup : public uiHorizonSetupGroup
{
public:
				~uiBaseHorizonSetupGroup();
    static void			initClass();

protected:
				uiBaseHorizonSetupGroup(uiParent*,const char*);
    static uiSetupGroup*	create(uiParent*,const char* typestr);
};


} // namespace MPE
