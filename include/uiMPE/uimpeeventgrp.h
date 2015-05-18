#ifndef uimpeeventgrp_h
#define uimpeeventgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "flatview.h"

#include "uidlggroup.h"
#include "uimpe.h"

class uiCheckList;
class uiFlatViewer;
class uiGenInput;
class uiPushButton;

namespace MPE
{

class HorizonAdjuster;
class SectionTracker;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiEventGroup : public uiDlgGroup
{ mODTextTranslationClass(uiEventGroup)
public:
				uiEventGroup(uiParent*,bool is2d);
				~uiEventGroup();

    void			setSectionTracker(SectionTracker*);

    void			setSeedPos(const Coord3&);

    NotifierAccess*		eventChangeNotifier()
				{ return &changed_; }

    bool			commitToTracker(bool& fieldchange) const;

protected:

    void			init();
    void			updateViewer();
    void			updateWindowLines();
    void			changeCB(CallBacker*);
    void			selEventType(CallBacker*);
    void			windowChangeCB(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			addStepPushedCB(CallBacker*);
    void			visibleDataChangeCB(CallBacker*);
    void			wvavdChgCB(CallBacker*);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);
    bool			mousedown_;

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiPushButton*		addstepbut_;
    uiGenInput*			extriffailfld_;
    uiGenInput*			nrzfld_;
    uiGenInput*			nrtrcsfld_;

    uiCheckList*		wvafld_;
    uiFlatViewer*		previewvwr_;

    FlatView::AuxData*		seeditm_;
    FlatView::AuxData*		minitm_;
    FlatView::AuxData*		maxitm_;

    bool			is2d_;
    Coord3			seedpos_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		adjuster_;

    Notifier<uiEventGroup>	changed_;
};

} // namespace MPE

#endif
