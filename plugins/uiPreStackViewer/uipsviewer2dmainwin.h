#ifndef uipsviewer2dmainwin_h
#define uipsviewer2dmainwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id: uipsviewer2dmainwin.h,v 1.1 2011-01-31 13:03:50 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"

#include "multiid.h"
#include "cubesampling.h"

namespace PreStackView
{
    class uiViewer2D;
    class uiViewer2DControl;
    class uiViewer2DPosDlg;

mClass uiViewer2DMainWin : public uiFlatViewMainWin
{
public:    
			uiViewer2DMainWin(uiParent*);

    uiViewer2D*		viewer2D() 	{ return viewer2d_; }
    void 		setMultiID(const MultiID&);

protected:

    MultiID 		mid_;
    uiViewer2DPosDlg* 	posdlg_;
    uiViewer2D*		viewer2d_;
    uiViewer2DControl*	control_;
    CubeSampling 	cs_;

    void		addGather(const BinID&); 

    void 		setUpView(CallBacker*);
    void 		posDlgPushed(CallBacker*);
};


mClass uiViewer2DControl : public uiFlatViewStdControl
{
public:
			uiViewer2DControl(uiParent*,uiFlatViewer&);

    void 		addGather(uiFlatViewer&);
    void 		removeGather(uiFlatViewer&);

    Notifier<uiViewer2DControl> posdlgcalled_;

protected:

    ObjectSet<uiFlatViewer> gathervwrs_;
    uiToolButton*    	posbut_;

    void		gatherPosCB(CallBacker*);
};


}; //namespace

#endif
