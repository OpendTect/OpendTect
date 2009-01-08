#ifndef uiflatviewpropdlg_h
#define uiflatviewpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.h,v 1.12 2009-01-08 07:14:05 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "flatview.h"
#include "iopar.h"
class uiFVWVAPropTab;
class uiFVVDPropTab;
class uiFVAnnotPropTab;
class BufferStringSet;
class uiGenInput;

		     
mClass uiFlatViewPropDlg : public uiTabStackDlg
{
public:
			uiFlatViewPropDlg(uiParent*,FlatView::Viewer&,
					  const CallBack& applcb,
					  const BufferStringSet* anns=0,
					  int selann=0, bool withwva=true);
    FlatView::Viewer&	viewer() 			{ return vwr_; }

    void		putAllToScreen();
    void		getAllFromScreen();

    int			selectedAnnot() const		{ return selannot_; }

protected:

    uiGenInput*		titlefld_;
    uiFVWVAPropTab*	wvatab_;
    uiFVVDPropTab*	vdtab_;
    uiFVAnnotPropTab*	annottab_;

    FlatView::Viewer&	vwr_;
    IOPar		initialpar_;
    int			selannot_;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    CallBack		applycb_;
    void		doApply(CallBacker*);

};

#endif
