#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "color.h"
#include "draw.h"
#include "valseriesevent.h"
#include "emseedpicker.h"

#include "uimpe.h"


class uiButtonGroup;
class uiColorInput;
class uiGenInput;
class uiPushButton;
class uiSlider;
class uiTabStack;


namespace MPE
{

class HorizonAdjuster;
class SectionTracker;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiHorizonSetupGroup : public uiSetupGroup
{ mODTextTranslationClass(uiHorizonSetupGroup)
public:
				uiHorizonSetupGroup(uiParent*,const char*);
				~uiHorizonSetupGroup();

    void			setSectionTracker(SectionTracker*);

    void			setMode(const EMSeedPicker::SeedModeOrder);
    int				getMode();
    void			setSeedPos(const Coord3&);
    void			setColor(const Color&);
    const Color&		getColor();
    void			setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D&	getMarkerStyle();

    NotifierAccess*		modeChangeNotifier()
				{ return &modechanged_; }
    NotifierAccess*		propertyChangeNotifier()
				{ return &propertychanged_; }
    NotifierAccess*		eventChangeNotifier()
				{ return &eventchanged_; }
    NotifierAccess*		similarityChangeNotifier()
				{ return &similaritychanged_; }

    virtual bool		commitToTracker() const
				{ bool b; return commitToTracker(b); }
    bool			commitToTracker(bool& fieldchange) const;

    void			showGroupOnTop(const char* grpnm);

protected:

    virtual void		initStuff();
    uiTabStack*			tabgrp_;

// Mode Group
    uiGroup*			createModeGroup();
    void			initModeGroup();
    void			seedModeChange(CallBacker*);

    uiButtonGroup*		modeselgrp_;


// Event Group
    uiGroup*			createEventGroup();
    void			initEventGroup();
    void			selEventType(CallBacker*);
    void			eventChangeCB(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			addStepPushedCB(CallBacker*);

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiPushButton*		addstepbut_;
    uiGenInput*			extriffailfld_;


// Correlation Group
    uiGroup*			createSimiGroup();
    void			initSimiGroup();
    void			selUseSimilarity(CallBacker*);
    void			similarityChangeCB(CallBacker*);

    uiGenInput*			usesimifld_;
    uiGenInput*			compwinfld_;
    uiGenInput*			simithresholdfld_;


// Property Group
    uiGroup*			createPropertyGroup();
    void			initPropertyGroup();
    void			colorChangeCB(CallBacker*);
    void			seedTypeSel(CallBacker*);
    void			seedColSel(CallBacker*);
    void			seedSliderMove(CallBacker*);

    uiColorInput*		colorfld_;
    uiGenInput*			seedtypefld_;
    uiColorInput*		seedcolselfld_;
    uiSlider*			seedsliderfld_;


    bool			is2d_;
    EMSeedPicker::SeedModeOrder	mode_;
    MarkerStyle3D		markerstyle_;

    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		horadj_;

    Notifier<uiHorizonSetupGroup> modechanged_;
    Notifier<uiHorizonSetupGroup> eventchanged_;
    Notifier<uiHorizonSetupGroup> similaritychanged_;
    Notifier<uiHorizonSetupGroup> propertychanged_;

    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();
};


mExpClass(uiMPE) uiBaseHorizonSetupGroup : public uiHorizonSetupGroup
{
public:
    static void			initClass();

protected:
				uiBaseHorizonSetupGroup(uiParent*,const char*);
    static uiSetupGroup*	create(uiParent*,const char* typestr);
};


} // namespace MPE

#endif

