#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uidlggroup.h"
#include "flatview.h"
#include "iopar.h"

class uiFVWVAPropTab;
class uiFVVDPropTab;
class uiFVAnnotPropTab;
class BufferStringSet;
class uiGenInput;

/*!
\brief FlatView properties dialog box.
*/

mExpClass(uiFlatView) uiFlatViewPropDlg : public uiTabStackDlg
{ mODTextTranslationClass(uiFlatViewPropDlg)
public:
			uiFlatViewPropDlg(uiParent*,FlatView::Viewer&,
					  const CallBack& applcb,
					  const BufferStringSet* anns=nullptr,
					  int selann=0,
					  bool withdynamictitle=false);
			~uiFlatViewPropDlg();

    FlatView::Viewer&	viewer() 			{ return vwr_; }

    void		putAllToScreen();
    void		getAllFromScreen();

    int			selectedAnnot() const		{ return selannot_; }
    void		fillPar(IOPar&) const;

protected:

    uiGenInput*		titlefld_;
    uiGenInput*		titleoptfld_;
    uiFVWVAPropTab*	wvatab_;
    uiFVVDPropTab*	vdtab_;
    uiFVAnnotPropTab*	annottab_;

    FlatView::Viewer&	vwr_;
    int			selannot_;

    void		finalizeCB(CallBacker*);
    void		titleChgCB(CallBacker*);
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;

    CallBack		applycb_;
    void		doApply(CallBacker*);

};
