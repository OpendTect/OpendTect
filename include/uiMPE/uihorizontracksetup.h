#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id: uihorizontracksetup.h,v 1.3 2006-05-04 20:31:58 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uimpe.h"
#include "valseriesevent.h"


class uiAttrSel;
class uiGenInput;
class uiPushButton;

namespace MPE
{

class HorizonAdjuster;


/*!\brief Horizon tracking setup dialog. */


class uiHorizonSetupGroup : public uiGroup
{
public:
				uiHorizonSetupGroup(uiParent*,SectionTracker*,
						    const Attrib::DescSet*);
				~uiHorizonSetupGroup();

    void			enableApplyButton(bool yn);
    NotifierAccess*		applyButtonPressed();

    bool			commitToTracker() const;

protected:
    void			initWin(CallBacker*);
    void			selUseSimilarity(CallBacker*);
    void			selAmpThresholdType(CallBacker*);

    uiAttrSel*			inpfld;
    uiGenInput*			usesimifld;
    uiGenInput*			thresholdtypefld;
    uiGenInput*			evfld;
    uiGenInput*			srchgatefld;
    uiGenInput*			ampthresholdfld;
    uiGenInput*			simithresholdfld;
    uiGenInput*			variancefld;
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


class uiHorizonSetupDialog : public uiSetupDialog
{
public:
    static void			initClass();
				/*!<Adds the class to the factory. */

    void			enableApplyButton(bool yn);
    NotifierAccess*		applyButtonPressed();
    bool			commitToTracker() const;

protected:
				uiHorizonSetupDialog(uiParent*,SectionTracker*,
						     const Attrib::DescSet*);
    static uiSetupDialog*	create(uiParent*,SectionTracker*,
				       const Attrib::DescSet*);

    bool			acceptOK(CallBacker*);

    uiHorizonSetupGroup*	grp;
};

} // namespace MPE

#endif
