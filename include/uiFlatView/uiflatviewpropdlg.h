#ifndef uiflatviewpropdlg_h
#define uiflatviewpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.h,v 1.9 2007-05-16 16:30:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "flatview.h"
class uiFVWVAPropTab;
class uiFVVDPropTab;
class uiFVAnnotPropTab;
class BufferStringSet;
class uiGenInput;

		     
class uiFlatViewPropDlg : public uiTabStackDlg
{
public:
			uiFlatViewPropDlg(uiParent*,FlatView::Viewer&,
					  const CallBack& applcb,
					  const BufferStringSet* anns=0,
					  int selann=0);
    FlatView::Viewer&	getViewer() 			{ return vwr_; }

    void		putAllToScreen();
    void		getAllFromScreen();

    int			selectedAnnot() const		{ return selannot_; }

protected:

    uiGenInput*		titlefld_;
    uiFVWVAPropTab*	wvatab_;
    uiFVVDPropTab*	vdtab_;
    uiFVAnnotPropTab*	annottab_;

    FlatView::Viewer&	vwr_;
    FlatView::Data	initialdata_;
    int			selannot_;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    CallBack		applycb_;
    void		doApply(CallBacker*);

};

#endif
