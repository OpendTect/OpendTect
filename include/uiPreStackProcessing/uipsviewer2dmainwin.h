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
#include "uipsviewer2dposdlg.h"
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

mExpClass(uiPreStackProcessing) uiViewer2DMainWin : public uiObjectItemViewWin, public uiFlatViewWin
{
public:    
			uiViewer2DMainWin(uiParent*,const char* title);

    virtual void 	start()		{ show(); }
    virtual void        setWinTitle( const char* t )    { setCaption(t); }

    virtual uiMainWin*  dockParent()    { return this; }
    virtual uiParent*   viewerParent()  { return this; }
    uiViewer2DControl*	control()	{ return control_; }
    virtual void	getGatherNames(BufferStringSet& nms) const	= 0;
    virtual bool	is2D() const	{ return false; }
    bool		isStored() const;
    TypeSet<BinID> 	getStartupPositions(const BinID& bid,
	    				    const StepInterval<int>& trcrg,
					    bool isinl) const;

    Notifier<uiViewer2DMainWin> seldatacalled_;

protected:

    TypeSet<GatherInfo>	gatherinfos_;
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
	    			      const GatherInfo&) 	{}
    virtual void	setGather(const GatherInfo& pos)	{} 
    void		setGatherView(uiGatherDisplay*,
	    			      uiGatherDisplayInfoHeader*);
    void 		setUpView();

    void		displayInfo(CallBacker*);
    void		doHelp(CallBacker*);
    void		posSlcChgCB(CallBacker*);
    virtual void	posDlgChgCB(CallBacker*)			=0;
    void		posDlgPushed(CallBacker*);		
    void 		dataDlgPushed(CallBacker*);
    void		showZAxis(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiStoredViewer2DMainWin : public uiViewer2DMainWin
{
public:
			uiStoredViewer2DMainWin(uiParent*,const char* title);

    void 		init(const MultiID&,const BinID& bid,bool isinl,
			     const StepInterval<int>&,const char* linename=0);
    void 		setIDs(const TypeSet<MultiID>&);
    void 		getIDs(TypeSet<MultiID>& mids) const 
			{ mids.copy( mids_ ); }
    void		getGatherNames(BufferStringSet& nms) const;
    virtual bool	is2D() const	{ return is2d_; }
    
protected:
    TypeSet<MultiID> 	mids_;

    bool		is2d_;
    BufferString	linename_;
    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const GatherInfo&);
    void		setGather(const GatherInfo&); 
    void		posDlgChgCB(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiSyntheticViewer2DMainWin : public uiViewer2DMainWin
{
public:
    			uiSyntheticViewer2DMainWin(uiParent*,const char* title);
    			~uiSyntheticViewer2DMainWin();
    void		setDataPacks(const TypeSet<PreStackView::GatherInfo>&);
    void		removeDataPacks();
    void		getGatherNames(BufferStringSet& nms) const;
    void		setGatherNames(const BufferStringSet& nms);
protected:
    void		posDlgChgCB(CallBacker*);

    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const GatherInfo&);
    void		setGather(const GatherInfo&); 
};


mClass(uiPreStackProcessing) uiViewer2DControl : public uiFlatViewStdControl
{
public:
			uiViewer2DControl(uiObjectItemView&,uiFlatViewer&);

    Notifier<uiViewer2DControl> posdlgcalled_;
    Notifier<uiViewer2DControl> datadlgcalled_;

    void 		removeAllViewers();

protected:

    uiObjectItemViewControl* objectitemctrl_;

    void		doPropertiesDialog(int vieweridx,bool dowva);

    void		applyProperties(CallBacker*);
    void		gatherPosCB(CallBacker*);
    void		gatherDataCB(CallBacker*);
};


}; //namespace

#endif

