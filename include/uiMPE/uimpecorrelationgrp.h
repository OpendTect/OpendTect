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

class uiCheckList;
class uiFlatViewer;
class uiGenInput;

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

    void			setSeedPos(const Coord3&);

    NotifierAccess*		correlationChangeNotifier()
				{ return &changed_; }

    bool			commitToTracker(bool& fieldchange) const;

protected:

    void			init();
    void			updateViewer();
    void			updateWindowLines();

    void			selUseCorrelation(CallBacker*);
    void			correlationChangeCB(CallBacker*);
    void			windowChangeCB(CallBacker*);
    void			visibleDataChangeCB(CallBacker*);
    void			wvavdChgCB(CallBacker*);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);
    bool			mousedown_;

    uiGenInput*			usecorrfld_;
    uiGenInput*			compwinfld_;
    uiGenInput*			corrthresholdfld_;
    uiGenInput*			nrzfld_;
    uiGenInput*			nrtrcsfld_;

    uiPreviewGroup*		previewgrp_;
    void			previewChgCB(CallBacker*);

    Coord3			seedpos_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		adjuster_;

    Notifier<uiCorrelationGroup> changed_;
};

} // namespace MPE

#endif
