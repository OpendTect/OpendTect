#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id: uihorizontracksetup.h,v 1.2 2006-02-27 11:20:17 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimpe.h"
#include "valseriesevent.h"


class uiAttrSel;
class uiGenInput;
class uiPushButton;

namespace MPE
{


class HorizonAdjuster;


/*!\brief Horizon tracking setup dialog. */

class uiHorizonSetupDialog :  public uiSetupDialog
{
public:
    static void		initClass();
			/*!<Adds the class to the factory. */
			~uiHorizonSetupDialog();

    void		enableApplyButton( bool yn );
    NotifierAccess*	applyButtonPressed();

    bool		commitToTracker() const;

protected:

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

				uiHorizonSetupDialog( uiParent*,SectionTracker*,
						      const Attrib::DescSet* );
    void			initWin(CallBacker*);
    void			selUseSimilarity(CallBacker*);
    void			selAmpThresholdType(CallBacker*);

    const Attrib::DescSet*	attrset_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		horadj_;

    bool			acceptOK(CallBacker*);

    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();
    static uiSetupDialog*	create( uiParent*, SectionTracker*,
				        const Attrib::DescSet* );
};


}; //Namespace

#endif
