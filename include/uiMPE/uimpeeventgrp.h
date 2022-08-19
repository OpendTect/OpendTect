#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "flatview.h"

#include "uidlggroup.h"
#include "uimpe.h"

class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiPushButton;

namespace MPE
{

class HorizonAdjuster;
class SectionTracker;
class uiPreviewGroup;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiEventGroup : public uiDlgGroup
{ mODTextTranslationClass(uiEventGroup)
public:
				uiEventGroup(uiParent*,bool is2d);
				~uiEventGroup();

    void			setSectionTracker(SectionTracker*);
    void			setSeedPos(const TrcKeyValue&);
    void			updateSensitivity(bool doauto);

    NotifierAccess*		changeNotifier()
				{ return &changed_; }

    bool			commitToTracker(bool& fieldchange) const;

protected:

    void			init();

    void			changeCB(CallBacker*);
    void			selEventType(CallBacker*);
    void			windowChangeCB(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			addStepPushedCB(CallBacker*);
    void			visibleDataChangeCB(CallBacker*);

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiPushButton*		addstepbut_;
    uiGenInput*			extriffailfld_;
    uiGenInput*			nrzfld_;
    uiGenInput*			nrtrcsfld_;
    uiLabel*			datalabel_;

    uiPreviewGroup*		previewgrp_;
    void			previewChgCB(CallBacker*);

    bool			is2d_;
    TrcKeyValue			seedpos_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		adjuster_;

    Notifier<uiEventGroup>	changed_;
};

} // namespace MPE
