#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
 RCS:           $Id: uihorizontracksetup.h 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "flatview.h"

#include "uidlggroup.h"
#include "uimpe.h"

class uiGenInput;
class uiLabeledSpinBox;

namespace MPE
{

class HorizonAdjuster;
class SectionTracker;
class uiPreviewGroup;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiCorrelationGroup : public uiDlgGroup
{ mODTextTranslationClass(uiCorrelationGroup)
public:
				uiCorrelationGroup(uiParent*,bool is2d);
				~uiCorrelationGroup();

    void			setSectionTracker(SectionTracker*);

    void			setSeedPos(const TrcKeyValue&);

    NotifierAccess*		changeNotifier()
				{ return &changed_; }

    bool			commitToTracker(bool& fieldchange) const;

protected:

    void			init();

    void			selUseCorrelation(CallBacker*);
    void			correlationChangeCB(CallBacker*);
    void			windowChangeCB(CallBacker*);
    void			visibleDataChangeCB(CallBacker*);

    uiGenInput*			usecorrfld_;
    uiGenInput*			compwinfld_;
    uiLabeledSpinBox*		corrthresholdfld_;
    uiGenInput*			nrzfld_;
    uiGenInput*			nrtrcsfld_;
    uiGenInput*			snapfld_;

    uiPreviewGroup*		previewgrp_;
    void			previewChgCB(CallBacker*);

    TrcKeyValue			seedpos_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		adjuster_;

    Notifier<uiCorrelationGroup> changed_;
};

} // namespace MPE

