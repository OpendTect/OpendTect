#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uigroup.h"
#include "stratlevel.h"
#include "stratsynth.h"

class uiFlatViewer;
class uiLineEdit;
class uiMultiFlatViewControl;
class uiSlider;
class uiStratLayModEditTools;
class uiStratSynthDispDSSel;
class uiSynthGenDlg;
class uiTextItem;

namespace FlatView { class AuxData; class Appearance; }
namespace PreStackView { class uiSyntheticViewer2DMainWin; }
namespace StratSynth { class SynthSpecificParsSet; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:

			uiStratSynthDisp(uiParent*,StratSynth::DataMgr&,
					 uiStratLayModEditTools&,uiSize);
			~uiStratSynthDisp();

    StratSynth::DataMgr& dataMgr()		{ return datamgr_; }
    const StratSynth::DataMgr& dataMgr() const	{ return datamgr_; }

    void		handleModelChange(bool full);
    void		setSelectedSequence(int);
    void		useDispPars(const IOPar&,od_uint32* =nullptr);
    void		setSavedViewRect();

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    uiFlatViewer*	viewer()		{ return vwr_; }
    uiMultiFlatViewControl* control()		{ return control_; }

    Notifier<uiStratSynthDisp> viewChanged;

    uiFlatViewer*	getViewerClone(uiParent*) const;
    void		addViewerToControl(uiFlatViewer&);

    void		makeInfoMsg(BufferString&,IOPar&);

protected:

    StratSynth::DataMgr& datamgr_;
    uiStratLayModEditTools& edtools_;
    uiSize		initialsz_;
    int			selseq_;
    uiWorldRect		initialboundingbox_;
    float		curoffs_	= 0.f;
    bool		canupdatedisp_ = true;

    uiMultiFlatViewControl* control_;
    ObjectSet<FlatView::AuxData> levelaux_;

    uiFlatViewer*	vwr_;
    uiLineEdit*		wvltfld_;
    uiTextItem*		modtypetxtitm_	= nullptr;
    uiStratSynthDispDSSel* wvaselfld_;
    uiStratSynthDispDSSel* vdselfld_;
    StratSynth::SynthSpecificParsSet& entries_;
    uiSynthGenDlg*	uidatamgr_	= nullptr;
    uiGroup*		psgrp_;
    uiSlider*		offsslider_;
    PreStackView::uiSyntheticViewer2DMainWin* psvwrwin_ = nullptr;

    void		createViewer(uiGroup*);
    void		setDefaultAppearance(FlatView::Appearance&);
    void		updFlds(bool full);
    void		updateEntries(bool full);
    void		updateDispPars(FlatView::Viewer::VwrDest,
				       od_uint32* =nullptr);
    void		updWvltFld();
    void		reDisp(bool preserveview=true);
    void		setViewerData(FlatView::Viewer::VwrDest,od_uint32& ctyp,
				      bool preserveview=true);
    void		drawLevels(od_uint32& ctyp);
    bool		curIsPS(FlatView::Viewer::VwrDest) const;
    void		handlePSViewDisp(FlatView::Viewer::VwrDest);
    void		handleChange(od_uint32);
    void		updateOffSliderTxt();
    void		setPSVwrData();

    int			dispEach() const;
    bool		dispFlattened() const;

    void		initGrp(CallBacker*);
    void		dataMgrCB(CallBacker*);
    void		newAddedCB(CallBacker*);
    void		newSelCB(CallBacker*);
    void		packSelCB(CallBacker*);
    void		wvaSelCB(CallBacker*);
    void		vdSelCB(CallBacker*);
    void		zoomChangedCB(CallBacker*);
    void		offsSliderChgCB(CallBacker*);
    void		viewPSCB(CallBacker*);
    void		setPSVwrDataCB(CallBacker*);

    void		viewChgCB(CallBacker*);
    void		lvlChgCB(CallBacker*);
    void		flatChgCB(CallBacker*);
    void		dispEachChgCB(CallBacker*);
    void		curModEdChgCB(CallBacker*);
    void		canvasResizeCB(CallBacker*);
    void		dispPropChgCB(CallBacker*);
    void		synthAddedCB(CallBacker*);
    void		synthRenamedCB(CallBacker*);
    void		synthRemovedCB(CallBacker*);

    friend class	uiStratSynthDispDSSel;

public:

    void		enableDispUpdate(bool yn);

};
