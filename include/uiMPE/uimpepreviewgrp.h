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

#include "uigroup.h"
#include "position.h"
#include "trckey.h"

class uiCheckList;
class uiFlatViewer;

namespace FlatView { class AuxData; }


namespace MPE
{

/*!\brief Viewer for previewing data around seed */

mExpClass(uiMPE) uiPreviewGroup : public uiGroup
{ mODTextTranslationClass(uiPreviewGroup)
public:
				uiPreviewGroup(uiParent*);
				~uiPreviewGroup();

    void			setSeedPos(const TrcKeyValue&);
    void			setDisplaySize(int nrtrcs,
					       const Interval<int>& zintv);
    void			setWindow(const Interval<int>&);
    Interval<int>		getManipWindow() const;

    Notifier<uiPreviewGroup>	windowChanged_;

protected:

    void			init();
    void			updateViewer();
    void			updateWindowLines();

    void			wvavdChgCB(CallBacker*);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);
    bool			calcNewWindow();
    bool			mousedown_;

    uiCheckList*		wvafld_;
    uiFlatViewer*		vwr_;

    FlatView::AuxData*		seeditm_;
    FlatView::AuxData*		seedline_;
    FlatView::AuxData*		minline_;
    FlatView::AuxData*		maxline_;

    TrcKeyValue			seedpos_;
    int				nrtrcs_;
    Interval<int>		zintv_;
    Interval<int>		winintv_;
    Interval<int>		manipwinintv_;

};

} // namespace MPE
