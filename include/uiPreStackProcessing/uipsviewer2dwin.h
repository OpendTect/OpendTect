#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uipsviewer2dposdlg.h"
#include "uiobjectitemviewwin.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"
#include "uistring.h"

#include "dbkey.h"
#include "trckeyzsampling.h"
#include "flatview.h"

class Gather;
class uiSlicePos2DView;
class uiColSeqSel;

namespace PreStack { class MuteDef; class ProcessManager;
		     class VelocityBasedAngleComputer; class AngleCompParams; }
namespace PreStackView
{
    class uiViewer2D;
    class uiViewer2DControl;
    class uiViewer2DPosDlg;
    class uiGatherDisplay;
    class uiGatherDisplayInfoHeader;
    class uiPSMultiPropDlg;

mExpClass(uiPreStackProcessing) PSViewAppearance : public FlatView::Appearance
{ mODTextTranslationClass(PSViewAppearance)
public:
    BufferString	datanm_;
    bool operator==( const PSViewAppearance& psapp ) const
    { return datanm_ == psapp.datanm_; }
};


mExpClass(uiPreStackProcessing) uiViewer2DWin : public uiObjectItemViewWin
					      , public uiFlatViewWin
{ mODTextTranslationClass(uiViewer2DWin);
public:

			~uiViewer2DWin();

    virtual void	start()		{ show(); }
    virtual void        setWinTitle(const uiString& t)
				    { setCaption(t); }

    virtual uiMainWin*  dockParent()    { return this; }
    virtual uiParent*   viewerParent()  { return this; }
    uiViewer2DControl*	control()	{ return control_; }
    virtual void	getGatherNames(BufferStringSet& nms) const	= 0;
    virtual bool	is2D() const	{ return false; }
    bool		isStored() const;
    void		getStartupPositions(const BinID& bid,
					    const StepInterval<int>& trcrg,
					    bool isinl, TypeSet<BinID>&) const;
    void		setAppearance(const FlatView::Appearance&,
				       int appidx=0);

    Notifier<uiViewer2DWin> seldatacalled_;
    const TypeSet<GatherInfo>&	gatherInfos() const	{ return gatherinfos_; }
    TypeSet<GatherInfo>&	gatherInfos()		{ return gatherinfos_; }

protected:

			uiViewer2DWin(uiParent*,const uiString&);

    TypeSet<GatherInfo>	gatherinfos_;
    TypeSet<int>	dpids_;
    ObjectSet<PreStack::MuteDef> mutes_;
    TypeSet<Color>	mutecolors_;
    TrcKeyZSampling	tkzs_;
    uiViewer2DPosDlg*	posdlg_;
    uiViewer2DControl*	control_;
    uiObjectItemViewAxisPainter* axispainter_;
    Interval<float>	zrg_;
    ObjectSet<uiGatherDisplay>	gd_;
    ObjectSet<uiGatherDisplayInfoHeader> gdi_;
    PreStack::ProcessManager*	preprocmgr_;
    TypeSet<PSViewAppearance>	appearances_;
    bool		hasangledata_;

    void		removeAllGathers();
    void		reSizeItems();
    virtual void	setGatherInfo(uiGatherDisplayInfoHeader* info,
				      const GatherInfo&)	{}
    virtual void	setGather(const GatherInfo& pos)	{}
    void		setGatherView(uiGatherDisplay*,
				      uiGatherDisplayInfoHeader*);
    RefMan<Gather> getAngleGather(const Gather& gather,
				       const Gather& angledata,
				       const Interval<int>& anglerange);
    DataPack::ID	getPreProcessedID(const GatherInfo&);
    void		setGatherforPreProc(const BinID& relbid,
					    const GatherInfo&);

    void		setUpView();
    void		clearAuxData();
    void		displayMutes();

    void		displayInfo(CallBacker*);
    void		doHelp(CallBacker*);
    virtual void	posDlgChgCB(CallBacker*)			=0;
    void		posDlgPushed(CallBacker*);
    void		posDlgClosed(CallBacker*);
    void		dataDlgPushed(CallBacker*);
    void		showZAxis(CallBacker*);
    void		loadMuteCB(CallBacker*);
    void		snapshotCB(CallBacker*);
    void		preprocessingCB(CallBacker*);
    void		applyPreProcCB(CallBacker*);
    void		propChangedCB(CallBacker*);
    void		prepareNewAppearances(BufferStringSet oldgathernms,
					      BufferStringSet newgathernms);
    bool		getStoredAppearance(PSViewAppearance&) const;
};


mExpClass(uiPreStackProcessing) uiStoredViewer2DWin : public uiViewer2DWin
{
public:
			uiStoredViewer2DWin(uiParent*,const uiString& title,
						bool is2d=false);

    void		init(const DBKey&,const BinID& bid,bool isinl,
			     const StepInterval<int>&,const char* linename=0);
    void		setIDs(const DBKeySet&);
    void		getIDs( DBKeySet& dbkys ) const
			{ dbkys = mids_; }
    void		getGatherNames(BufferStringSet& nms) const;
    virtual bool	is2D() const	{ return is2d_; }
    const char*		lineName() const	{ return linename_; }
    void		angleGatherCB(CallBacker*);
    void		angleDataCB(CallBacker*);

protected:
    DBKeySet	mids_;

    bool		is2d_;
    BufferString	linename_;
    PreStack::AngleCompParams* angleparams_;
    bool		doanglegather_;
    uiSlicePos2DView*	slicepos_;

    void		displayAngle();
    bool		getAngleParams();
    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
				      const GatherInfo&);
    void		setGather(const GatherInfo&);
    void		setUpNewPositions(bool isinl,const BinID& posbid,
				       const StepInterval<int>& trcrg);
    void		setUpNewSlicePositions();
    void		setUpNewIDs();
    void		convAngleDataToDegrees(
				Gather* angledata) const;
    RefMan<Gather> getAngleData(DataPack::ID gatherid);
    void		posDlgChgCB(CallBacker*);
    void		posSlcChgCB(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiSyntViewer2DWin : public uiViewer2DWin
{
public:

			uiSyntViewer2DWin(uiParent*,const uiString& title);
			~uiSyntViewer2DWin();
    void		setGathers(const TypeSet<PreStackView::GatherInfo>&);
    void		setGathers(const TypeSet<PreStackView::GatherInfo>&,
				   bool getstaruppositions);
    void		removeGathers();
    void		getGatherNames(BufferStringSet& nms) const;
    void		setGatherNames(const BufferStringSet& nms);

protected:

    void		posDlgChgCB(CallBacker*);

    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
				      const GatherInfo&);
    void		setGather(const GatherInfo&);

};


mClass(uiPreStackProcessing) uiViewer2DControl : public uiFlatViewStdControl
{ mODTextTranslationClass(uiViewer2DControl)
public:
			uiViewer2DControl(uiObjectItemView&,uiFlatViewer&,
					  bool isstored);
			~uiViewer2DControl();

    Notifier<uiViewer2DControl> posdlgcalled_;
    Notifier<uiViewer2DControl> datadlgcalled_;
    Notifier<uiViewer2DControl> propChanged;

    void			removeAllViewers();
    const FlatView::DataDispPars& dispPars() const    { return app_.ddpars_; }
    FlatView::DataDispPars&	dispPars()	      { return app_.ddpars_; }
    const FlatView::Annotation& annot() const	      { return app_.annot_; }
    FlatView::Annotation&	annot()		      { return app_.annot_; }
    void			setGatherInfos(const TypeSet<GatherInfo>&);
    PSViewAppearance		curViewerApp();

protected:

    uiObjectItemViewControl*	objectitemctrl_;
    FlatView::Appearance	app_;
    uiColSeqSel*		colseqsel_;
    TypeSet<GatherInfo>		gatherinfos_;
    PreStackView::uiPSMultiPropDlg* pspropdlg_;
    bool			isstored_;

    void		doPropertiesDialog(int vieweridx);

    void		applyProperties(CallBacker*);
    void		gatherPosCB(CallBacker*);
    void		gatherDataCB(CallBacker*);
    void		coltabChg(CallBacker*);
    void		updateColTabCB(CallBacker*);
    void		propertiesDlgCB(CallBacker*);
};


}; //namespace
