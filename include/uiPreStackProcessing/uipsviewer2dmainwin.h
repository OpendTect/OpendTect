#ifndef uipsviewer2dmainwin_h
#define uipsviewer2dmainwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uiobjectitemviewwin.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"

#include "multiid.h"
#include "cubesampling.h"


class uiSlicePos2DView;

namespace PreStack { class Gather; }
namespace PreStackView
{
    class uiViewer2D;
    class uiViewer2DControl;
    class uiViewer2DPosDlg;
    class uiGatherDisplay;
    class uiGatherDisplayInfoHeader;

mClass(uiPreStackProcessing) uiViewer2DMainWin : public uiObjectItemViewWin, public uiFlatViewWin
{
public:    
			uiViewer2DMainWin(uiParent*,const char* title);

    virtual void 	start()		{ show(); }
    virtual void        setWinTitle( const char* t )    { setCaption(t); }

    virtual uiMainWin*  dockParent()    { return this; }
    virtual uiParent*   viewerParent()  { return this; }
    uiViewer2DControl*	control()	{ return control_; }

    Notifier<uiViewer2DMainWin> seldatacalled_;

protected:

    TypeSet<int> 	dpids_;
    CubeSampling 	cs_;
    uiSlicePos2DView*	slicepos_;
    uiViewer2DPosDlg* 	posdlg_;
    uiViewer2DControl*	control_;
    uiObjectItemViewAxisPainter* axispainter_;
    Interval<float>	zrg_;

    void		removeAllGathers();
    void		reSizeItems();
    virtual void	setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const BinID&,int) {}
    virtual void	setGathers(const BinID& pos)		{} 
    void		setGatherView(uiGatherDisplay*,
	    			      uiGatherDisplayInfoHeader*);
    void 		setUpView();

    void		displayInfo(CallBacker*);
    void		doHelp(CallBacker*);
    void		posSlcChgCB(CallBacker*);
    void		posDlgChgCB(CallBacker*);
    virtual void	posDlgPushed(CallBacker*)		{}
    void 		dataDlgPushed(CallBacker*);
    void		showZAxis(CallBacker*);
};


mClass(uiPreStackProcessing) uiStoredViewer2DMainWin : public uiViewer2DMainWin
{
public:
			uiStoredViewer2DMainWin(uiParent*,const char* title);

    void 		init(const MultiID&,int gatherid,bool isinl,
			    const StepInterval<int>&,const char* linename=0);
    void 		setIDs(const TypeSet<MultiID>&);
    void 		getIDs(TypeSet<MultiID>& mids) const 
			{ mids.copy( mids_ ); }
    
    bool		is2D() const 		{ return is2d_; }
protected:
    TypeSet<MultiID> 	mids_;

    bool		is2d_;
    BufferString	linename_;
    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const BinID&,int);
    void		setGathers(const BinID& pos); 
    
    void 		posDlgPushed(CallBacker*);
};


mStruct(uiPreStackProcessing) GatherInfo
{
    int		dpid_;
    BufferString	gathernm_;
    BinID		bid_;
    bool operator==( const GatherInfo& info ) const
    { return gathernm_==info.gathernm_ && bid_==info.bid_; }
};

mClass(uiPreStackProcessing) uiSyntheticViewer2DMainWin : public uiViewer2DMainWin
{
public:
    			uiSyntheticViewer2DMainWin(uiParent*,const char* title);
    			~uiSyntheticViewer2DMainWin();
    void		setDataPacks(const TypeSet<GatherInfo>&);
    void		removeDataPacks();
protected:
    TypeSet<GatherInfo>	dpinfos_;
    
    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const BinID&,int idx);
    void		setGathers(const BinID& pos); 
};


mClass(uiPreStackProcessing) uiViewer2DControl : public uiFlatViewStdControl
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

