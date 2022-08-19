#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "uigroup.h"
#include "position.h"
#include "trckeyvalue.h"

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
