#ifndef uipsviewer2dmainwin_h
#define uipsviewer2dmainwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id: uipsviewer2dmainwin.h,v 1.9 2012-06-28 11:29:14 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiobjectitemviewwin.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"

#include "multiid.h"
#include "cubesampling.h"


class uiSlicePos2DView;

namespace PreStackView
{
    class uiViewer2D;
    class uiViewer2DControl;
    class uiViewer2DPosDlg;

mClass uiViewer2DMainWin : public uiObjectItemViewWin, public uiFlatViewWin
{
public:    
			uiViewer2DMainWin(uiParent*,const char* title);

    void 		init(const MultiID&,int gatherid,bool isinl,
			    const StepInterval<int>&,const char* linename=0);
    virtual void 	start()		{ show(); }
    virtual void        setWinTitle( const char* t )    { setCaption(t); }

    void 		setIDs(const TypeSet<MultiID>&);
    void 		getIDs(TypeSet<MultiID>& mids) const 
			{ mids.copy( mids_ ); }

    bool		is2D() const 		{ return is2d_; }

    virtual uiMainWin*  dockParent()    { return this; }
    virtual uiParent*   viewerParent()  { return this; }

    Notifier<uiViewer2DMainWin> seldatacalled_;

protected:

    bool		isinl_;
    TypeSet<MultiID> 	mids_;
    CubeSampling 	cs_;
    uiSlicePos2DView*	slicepos_;
    uiViewer2DPosDlg* 	posdlg_;
    uiViewer2DControl*	control_;
    uiObjectItemViewAxisPainter* axispainter_;
    Interval<float>	zrg_;

    bool		is2d_;
    BufferString	linename_;

    void		removeAllGathers();
    void		reSizeItems();
    void		setGathers(const BinID& pos); 
    void 		setUpView();

    void		displayInfo(CallBacker*);
    void		doHelp(CallBacker*);
    void		posSlcChgCB(CallBacker*);
    void		posDlgChgCB(CallBacker*);
    void 		posDlgPushed(CallBacker*);
    void 		dataDlgPushed(CallBacker*);
    void		showZAxis(CallBacker*);
};


mClass uiViewer2DControl : public uiFlatViewStdControl
{
public:
			uiViewer2DControl(uiObjectItemView&,uiFlatViewer&);

    Notifier<uiViewer2DControl> posdlgcalled_;
    Notifier<uiViewer2DControl> datadlgcalled_;

    void 		removeAllViewers();

protected:

    uiToolButton*    	posbut_;
    uiToolButton*    	databut_;
    uiObjectItemViewControl* objectitemctrl_;


    void		doPropertiesDialog(int vieweridx,bool dowva);

    void		applyProperties(CallBacker*);
    void		gatherPosCB(CallBacker*);
    void		gatherDataCB(CallBacker*);
};


}; //namespace

#endif
