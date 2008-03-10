#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id: uihorizontracksetup.h,v 1.8 2008-03-10 15:40:58 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimpe.h"
#include "valseriesevent.h"


class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTabStack;

namespace MPE
{

class HorizonAdjuster;
class SectionTracker;


/*!\brief Horizon tracking setup dialog. */


class uiHorizonSetupGroup : public uiSetupGroup
{
public:
    static void			initClass();
				/*!<Adds the class to the factory. */

				~uiHorizonSetupGroup();

    void			setSectionTracker(SectionTracker*);
    void			setAttribSet(const Attrib::DescSet*);
    bool			commitToTracker(bool& fieldchange) const;

protected:
				uiHorizonSetupGroup(uiParent*,
						    const Attrib::DescSet*);
    static uiSetupGroup*	create(uiParent*,const char* typestr,
	    			       const Attrib::DescSet*);

    uiGroup*			createEventGroup();
    void			initEventGroup();
    uiGroup*			createSimiGroup();
    void			initSimiGroup();
    uiGroup*			createAutoGroup();
    void			initAutoGroup();

    void			selUseSimilarity(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			selEventType(CallBacker*);

    uiTabStack*			tabgrp_;
    uiAttrSel*			inpfld;
    uiGenInput*			usesimifld;
    uiGenInput*			thresholdtypefld;
    uiGenInput*			evfld;
    uiGenInput*			srchgatefld;
    uiGenInput*			ampthresholdfld;
    uiGenInput*			simithresholdfld;
    uiGenInput*			compwinfld;
    uiGenInput*			extriffailfld;
    uiGenInput*			seedonlypropfld;
    uiGroup*			maingrp;
    uiPushButton*		applybut;

    const Attrib::DescSet*	attrset_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		horadj_;

    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();
};


} // namespace MPE

#endif
