#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Dec 2006
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

