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

#include "color.h"
#include "draw.h"
#include "valseriesevent.h"
#include "emseedpicker.h"

#include "uimpe.h"


class uiAttrSel;
class uiButtonGroup;
class uiColorInput;
class uiGenInput;
class uiPushButton;
class uiSliderExtra;
class uiTabStack;


namespace MPE
{

class HorizonAdjuster;
class SectionTracker;


/*!\brief Horizon tracking setup dialog. */

mClass uiHorizonSetupGroup : public uiSetupGroup
{
public:
    //static void			initClass();
				/*!<Adds the class to the factory. */
    				uiHorizonSetupGroup(uiParent*,
						    const Attrib::DescSet*,
						    const char*);
				~uiHorizonSetupGroup();

    void			setSectionTracker(SectionTracker*);
    void			setAttribSet(const Attrib::DescSet*);

    void                        setMode(const EMSeedPicker::SeedModeOrder);
    const int                   getMode();
    void                        setColor(const Color&);
    const Color&                getColor();
    void                        setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D&        getMarkerStyle();

    void			setAttribSelSpec(const Attrib::SelSpec*);
    bool			isSameSelSpec(const Attrib::SelSpec*) const;

    NotifierAccess*		modeChangeNotifier()	
    				{ return &modechanged_; }
    NotifierAccess*		propertyChangeNotifier()	
				{ return &propertychanged_; }
    NotifierAccess*		eventChangeNotifier()
    				{ return &eventchanged_; }
    NotifierAccess*		similartyChangeNotifier()
				{ return &similartychanged_; }

    virtual bool		commitToTracker() const
				{ bool b; return commitToTracker(b); }
    bool			commitToTracker(bool& fieldchange) const;

protected:
//				uiHorizonSetupGroup(uiParent*,
//						    const Attrib::DescSet*,
//						    const char*);
    //static uiSetupGroup*	create(uiParent*,const char* typestr,
//	    			       const Attrib::DescSet*);

    uiGroup*			createModeGroup();
    void			initModeGroup();
    uiGroup*			createEventGroup();
    void			initEventGroup();
    uiGroup*			createSimiGroup();
    void			initSimiGroup();
    uiGroup*			createPropertyGroup();
    void			initPropertyGroup();

    virtual void		initStuff();

    void			selUseSimilarity(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			selEventType(CallBacker*);
    void                	seedModeChange(CallBacker*);
    void			eventChangeCB(CallBacker*);
    void			similartyChangeCB(CallBacker*);
    void			colorChangeCB(CallBacker*);
    void			seedTypeSel(CallBacker*);
    void			seedSliderMove(CallBacker*);
    void			seedColSel(CallBacker*);
    void			addStepPushedCB(CallBacker*);

    uiTabStack*			tabgrp_;
    uiButtonGroup*      	modeselgrp_;
    uiAttrSel*			inpfld_;
    uiGenInput*			usesimifld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			ampthresholdfld_;
    uiPushButton*		addstepbut_;
    uiGenInput*			simithresholdfld_;
    uiGenInput*			compwinfld_;
    uiGenInput*			extriffailfld_;
    uiGroup*			maingrp_;
    uiPushButton*		applybut_;
    uiColorInput*		colorfld_;
    uiSliderExtra*      	seedsliderfld_;
    uiGenInput*         	seedtypefld_;
    uiColorInput*       	seedcolselfld_;

    bool			is2d_;
    EMSeedPicker::SeedModeOrder	mode_;
    MarkerStyle3D       	markerstyle_;

    const Attrib::DescSet*	attrset_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		horadj_;

    Notifier<uiHorizonSetupGroup> modechanged_;
    Notifier<uiHorizonSetupGroup> eventchanged_;
    Notifier<uiHorizonSetupGroup> similartychanged_;
    Notifier<uiHorizonSetupGroup> propertychanged_;
    
    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();
};


mClass uiBaseHorizonSetupGroup : public uiHorizonSetupGroup
{
public:
    static void			initClass();
    
protected:
    				uiBaseHorizonSetupGroup(uiParent*,
							const Attrib::DescSet*,
							const char*);
    static uiSetupGroup*        create(uiParent*,const char* typestr,
	    			       const Attrib::DescSet*);
};


} // namespace MPE

#endif
