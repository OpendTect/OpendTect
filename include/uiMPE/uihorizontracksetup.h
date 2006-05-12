#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id: uihorizontracksetup.h,v 1.4 2006-05-12 09:51:14 cvsnanne Exp $
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
    bool			commitToTracker() const;

protected:
				uiHorizonSetupGroup(uiParent*,
						    const Attrib::DescSet*);
    static uiSetupGroup*	create(uiParent*,const char* typestr,
	    			       const Attrib::DescSet*);

    uiGroup*			createEventGroup();
    void			initEventGroup();
    uiGroup*			createSimiGroup();
    void			initSimiGroup();

    void			selUseSimilarity(CallBacker*);
    void			selAmpThresholdType(CallBacker*);

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
