#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"
#include "iopar.h"

class EventTracker;
class uiGenInput;

/*!User interface for EventTracker. */


mExpClass(uiTools) uiEventTracker : public uiDlgGroup
{ mODTextTranslationClass(uiEventTracker);
public:
    			uiEventTracker(uiParent*,EventTracker&,
				bool hideeventtype=false,
				bool immediateupdate=false);
			~uiEventTracker();

    bool		acceptOK() override;
    bool		rejectOK() override;

protected:
    void			selEventType(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			selUseSimilarity(CallBacker*);
    void			changeCB(CallBacker*);
    bool			updateTracker(bool domsg);

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiGenInput*			alloweddifffld_;

    uiGenInput*			usesimifld_;
    uiGenInput*			simithresholdfld_;
    uiGenInput*			compwinfld_;

    bool			immediateupdate_;

    EventTracker&		tracker_;
    IOPar			restorepars_;
};
