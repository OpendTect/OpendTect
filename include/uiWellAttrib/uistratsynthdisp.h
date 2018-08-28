#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uiflatviewslicepos.h"
#include "uistring.h"
#include "stratlevel.h"
#include "uistratsimplelaymoddisp.h"
#include "uitaskrunnerprovider.h"

class TimeDepthModel;
class SeisTrcBuf;
class SyntheticData;
class PropertyRef;
class PropertyRefSelection;
class Wavelet;
class uiButton;
class uiComboBox;
class uiFlatViewer;
class uiMultiFlatViewControl;
class uiPushButton;
class uiStratLayerModel;
class uiSynthGenDlg;
class uiSynthSlicePos;
class uiTextItem;
class uiToolButton;
class uiToolButtonSetup;
class uiWaveletIOObjSel;
namespace Strat { class LayerModel; class LayerModelProvider; }
namespace StratSynth { class DataMgr; }
namespace FlatView { class AuxData; }
namespace PreStackView { class uiSyntheticViewer2DMainWin; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:

    typedef ObjectSet<const TimeDepthModel>	T2DModelSet;
    typedef Strat::Level::ID			LevelID;
    typedef StratSynth::DataMgr			DataMgr;

			uiStratSynthDisp(uiParent*,uiStratLayerModel&,
					 const Strat::LayerModelProvider&);
			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;
    DBKey		waveletID() const;
    const Wavelet*	getWavelet() const;
    inline const DataMgr& curDM() const
			{ return *(!useed_ ? datamgr_ : eddatamgr_); }
    inline DataMgr&	curDM()
			{ return *(!useed_ ? datamgr_ : eddatamgr_); }
    inline const DataMgr& altDM() const
			{ return *(useed_ ? datamgr_ : eddatamgr_); }
    const DataMgr&	normalDM() const	{ return *datamgr_; }
    const DataMgr&	editDM() const		{ return *eddatamgr_; }

    const ObjectSet<SyntheticData>& getSynthetics() const;
    RefMan<SyntheticData>	getCurrentSyntheticData(bool wva=true) const;
    RefMan<SyntheticData>	getSyntheticData(const char* nm);
    const PropertyRefSelection&	modelPropertyRefs() const;

    const T2DModelSet*	d2TModels() const;

    void		setFlattened(bool flattened,bool trigger=true);
    void		updateMarkers();
    void		setSelectedTrace(int);
    void		setDispEach(int);
    void		setZDataRange(const Interval<double>&,bool indpt);
    void		setDisplayZSkip(float zskip,bool withmodchg);
    void		displayFRText(bool yn,bool isbrine);

    const uiWorldRect&	curView(bool indepth) const;
    void		setZoomView(const uiWorldRect&);

    uiFlatViewer*	viewer()		{ return vwr_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	viewChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;
    Notifier<uiStratSynthDisp>	synthsChanged;
    Notifier<uiStratSynthDisp>	dispParsChanged;

    void		addTool(const uiToolButtonSetup&);
    void		addViewerToControl(uiFlatViewer&);

    bool		haveUserScaleWavelet();
    void		displaySynthetic(ConstRefMan<SyntheticData>);
    void		reDisplayPostStackSynthetic(bool wva=true);
    void		cleanSynthetics();
    float		centralTrcShift() const;
    void		setCurrentSynthetic(bool wva);
    void		setSelectedLevel( LevelID lvlid ) { sellvlid_ = lvlid; }
    bool		prepareElasticModel();

    uiMultiFlatViewControl* control()	{ return control_; }

    void		fillPar(IOPar&) const;
    void		fillPar(IOPar&,bool) const;
    bool		usePar(const IOPar&);
    void		makeInfoMsg(uiString& msg,IOPar&);

    void		showFRResults();
    void		setAutoUpdate( bool yn )  { autoupdate_ = yn; }
    void		setForceUpdate( bool yn ) { forceupdate_ = yn; }
    bool		doForceUpdate() const	  { return forceupdate_; }
    void		setUseEdited( bool yn )	  { useed_ = yn; }
    bool		isEditUsed() const	  { return useed_; }
    void		setDiffData();
    void		resetRelativeViewRect();
    void		updateRelativeViewRect();
    void		setRelativeViewRect(const uiWorldRect& relwr);
    const uiWorldRect&	getRelativeViewRect() const	{ return relzoomwr_; }
    void		setSavedViewRect();
    LevelID		selectedLevelID()	    { return sellvlid_; }
    uiGroup*		getDisplayClone(uiParent*) const;

protected:

    int					longestaimdl_;
    DataMgr*				datamgr_;
    DataMgr*				eddatamgr_;
    const Strat::LayerModelProvider&	lmp_;
    uiWorldRect				relzoomwr_;
    mutable uiWorldRect			savedzoomwr_;
    int					selectedtrace_;
    int					dispeach_;
    float				dispskipz_;
    bool				dispflattened_;
    bool				autoupdate_;
    bool				forceupdate_;
    bool				useed_;
    LevelID				sellvlid_;

    const T2DModelSet*			d2tmodels_;
    RefMan<SyntheticData>		currentwvasynthetic_;
    RefMan<SyntheticData>		currentvdsynthetic_;

    uiMultiFlatViewControl*		control_;
    FlatView::AuxData*			selectedtraceaux_;
    ObjectSet<FlatView::AuxData>	levelaux_;
    uiTaskRunnerProvider		trprov_;

    uiGroup*				topgrp_;
    uiGroup*				datagrp_;
    uiGroup*				prestackgrp_;
    uiWaveletIOObjSel*			wvltfld_;
    uiFlatViewer*			vwr_;
    uiPushButton*			scalebut_;
    uiButton*				lasttool_;
    uiTextItem*				frtxtitm_;
    uiToolButton*			prestackbut_;
    uiComboBox*				wvadatalist_;
    uiComboBox*				vddatalist_;
    uiSynthGenDlg*			synthgendlg_;
    uiSynthSlicePos*			offsetposfld_;

    PreStackView::uiSyntheticViewer2DMainWin*	prestackwin_;

    void		showInfoMsg(bool foralt);
    void		handleFlattenChange();
    void		setCurrentWavelet();
    void		fillPar(IOPar&,const DataMgr&) const;
    void		doModelChange(CallBacker*);
    const SeisTrcBuf&	curTrcBuf() const;
    void		getCurD2TModels(ConstRefMan<SyntheticData>,
				    T2DModelSet&,float offset = 0.0f) const;
    void		reSampleTraces(ConstRefMan<SyntheticData>,
				       SeisTrcBuf&) const;
    void		updateFields();
    void		updateSynthetic(const char* nm,bool wva);
    void		updateSyntheticList(bool wva);
    void		copySyntheticDispPars();
    void		setDefaultAppearance(FlatView::Appearance&);
    inline DataMgr&	altDM()
			{ return *(useed_ ? datamgr_ : eddatamgr_); }

    void		drawLevels();
    void		displayPreStackSynthetic(ConstRefMan<SyntheticData>);
    void		displayPostStackSynthetic(ConstRefMan<SyntheticData>,
						  bool wva=true);
    void		updateTextPosCB(CallBacker*);

    void		setPreStackMapper();
    void		setAbsoluteViewRect(const uiWorldRect& abswr);
    void		getAbsoluteViewRect(uiWorldRect& abswr) const;

    void		addEditSynth(CallBacker*);
    void		exportSynth(CallBacker*);
    void		wvDataSetSel(CallBacker*);
    void		vdDataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		genNewSynthetic(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		viewChg(CallBacker*);
    void		parsChangedCB(CallBacker*);
    void		syntheticRemoved(CallBacker*);
    void		syntheticDisabled(CallBacker*);
    void		syntheticChanged(CallBacker*);
    void		selPreStackDataCB(CallBacker*);
    void		preStackWinClosedCB(CallBacker*);
};


mExpClass(uiWellAttrib) uiSynthSlicePos : public uiGroup
{ mODTextTranslationClass(uiSynthSlicePos);
public:
			uiSynthSlicePos(uiParent*,const uiString& lbltxt);

    Notifier<uiSynthSlicePos>	positionChg;
    void		setLimitSampling(StepInterval<float>);
    int			getValue() const;
    void		setValue(int) const;

protected:
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;

    void		slicePosChg( CallBacker* );
    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);

    StepInterval<float>	limitsampling_;
};
