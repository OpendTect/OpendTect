#ifndef uimpecorrelationgrp_h
#define uimpecorrelationgrp_h

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


class uiFlatViewer;
class uiGenInput;

namespace MPE
{

class HorizonAdjuster;
class SectionTracker;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiCorrelationGroup : public uiDlgGroup
{ mODTextTranslationClass(uiCorrelationGroup)
public:
				uiCorrelationGroup(uiParent*,bool is2d);
				~uiCorrelationGroup();

    void			setSectionTracker(SectionTracker*);

    void			setSeedPos(const Coord3&);

    NotifierAccess*		correlationChangeNotifier()
				{ return &changed_; }

    bool			commitToTracker(bool& fieldchange) const;

protected:

    void			init();
    void			updateViewer();
    void			selUseCorrelation(CallBacker*);
    void			correlationChangeCB(CallBacker*);
    void			windowChangeCB(CallBacker*);
    void			visibleDataChangeCB(CallBacker*);

    uiGenInput*			usecorrfld_;
    uiFlatViewer*		previewvwr_;
    uiGenInput*			compwinfld_;
    uiGenInput*			corrthresholdfld_;
    uiGenInput*			nrzfld_;
    uiGenInput*			nrtrcsfld_;

    FlatView::AuxData*		seeditm_;
    FlatView::AuxData*		minitm_;
    FlatView::AuxData*		maxitm_;

    Coord3			seedpos_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		adjuster_;

    Notifier<uiCorrelationGroup> changed_;
};

} // namespace MPE

#endif
