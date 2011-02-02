#ifndef uipsviewer2dmainwin_h
#define uipsviewer2dmainwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id: uipsviewer2dmainwin.h,v 1.2 2011-02-02 09:54:23 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"

#include "multiid.h"
#include "cubesampling.h"


class uiCheckBox;
class uiToolButton;
class uiSlicePos2DView;
class uiSliderExtra;

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
    void 		init(const MultiID&,int gatherid,bool isinl);

protected:

    bool		isinl_;
    bool		is2d_;
    MultiID 		mid_;
    uiViewer2DPosDlg* 	posdlg_;
    uiViewer2D*		viewer2d_;
    uiViewer2DControl*	control_;
    CubeSampling 	cs_;
    uiSlicePos2DView*	slicepos_;

    uiSliderExtra*	versliderfld_;
    uiSliderExtra*	horsliderfld_;
    uiCheckBox*		zoomratiofld_;
    int			startwidth_;
    int			startheight_;

    void		addGather(const BinID&); 
    void		makeSliders();
    void 		setUpView();

    void		reSizeSld(CallBacker*);
    void		posSlcChgCB(CallBacker*);
    void		posDlgChgCB(CallBacker*);
    void 		posDlgPushed(CallBacker*);
};


mClass uiViewer2DControl : public uiFlatViewStdControl
{
public:
			uiViewer2DControl(uiParent*,uiFlatViewer&);

    void 		addGather(uiFlatViewer&);
    void 		removeGather(uiFlatViewer&);
    void 		removeAllGathers();

    Notifier<uiViewer2DControl> posdlgcalled_;

protected:

    ObjectSet<uiFlatViewer> gathervwrs_;
    uiToolButton*    	posbut_;

    void		gatherPosCB(CallBacker*);
};


}; //namespace

#endif
