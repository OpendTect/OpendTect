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
#include "stratlevel.h"
#include "stratsynthdatamgr.h"

class uiFlatViewer;
class uiLineEdit;
class uiMultiFlatViewControl;
class uiSlider;
class uiStratLayModEditTools;
class uiStratSynthDispDSSel;
class uiStratSynthDataMgr;
class uiPSViewer2DWin;
class uiTextItem;
namespace FlatView { class AuxData; class Appearance; }
namespace PreStackView { class uiSyntViewer2DWin; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:

    typedef Strat::Level::ID			LevelID;
    typedef StratSynth::DataMgr			DataMgr;
    typedef Strat::LayerModel			LayerModel;
    typedef uiStratLayModEditTools		uiEdTools;
    typedef DataMgr::ZValueSet			ZValueSet;
    typedef PreStackView::uiSyntViewer2DWin	uiPSViewer2DWin;

			uiStratSynthDisp(uiParent*,DataMgr&,uiEdTools&,uiSize);
			~uiStratSynthDisp();

    DataMgr&		dataMgr()		{ return datamgr_; }
    const DataMgr&	dataMgr() const		{ return datamgr_; }

    void		handleModelChange();
    void		setSelectedSequence(int);

    uiFlatViewer*	viewer()	{ return vwr_; }
    uiMultiFlatViewControl* control()	{ return control_; }

    Notifier<uiStratSynthDisp>	viewChanged;
    Notifier<uiStratSynthDisp>	elasticPropsSelReq;

    uiFlatViewer*	getViewerClone(uiParent*) const;
    void		addViewerToControl(uiFlatViewer&);

    void		makeInfoMsg(uiString&,IOPar&);

protected:

    typedef FlatView::Appearance    Appearance;

    DataMgr&		datamgr_;
    uiEdTools&		edtools_;
    uiSize		initialsz_;
    int			selseq_;
    uiWorldRect		initialboundingbox_;
    float		curoffs_		= 0.f;

    uiMultiFlatViewControl* control_;
    ObjectSet<FlatView::AuxData> levelaux_;

    uiFlatViewer*	vwr_;
    uiLineEdit*		wvltfld_;
    uiTextItem*		modtypetxtitm_		= 0;
    uiStratSynthDispDSSel* wvaselfld_;
    uiStratSynthDispDSSel* vdselfld_;
    uiStratSynthDataMgr* uidatamgr_		= 0;
    uiGroup*		psgrp_;
    uiSlider*		offsslider_;
    uiPSViewer2DWin*	psvwrwin_		= 0;

    void		createViewer(uiGroup*);
    void		setDefaultAppearance(Appearance&);
    void		updFlds();
    void		updWvltFld();
    void		reDisp(bool preserveview=true);
    void		setViewerData(bool wva,bool preserveview=true);
    void		drawLevels();
    bool		curIsPS() const;
    void		setPSVwrData();
    void		handlePSViewDisp();

    int			dispEach() const;
    bool		dispFlattened() const;

    void		initGrp(CallBacker*);
    void		elPropEdCB(CallBacker*);
    void		dataMgrCB(CallBacker*);
    void		expSynthCB(CallBacker*);
    void		wvaSelCB(CallBacker*);
    void		vdSelCB(CallBacker*);
    void		offsSliderChgCB(CallBacker*);
    void		viewPSCB(CallBacker*);
    void		setPSVwrDataCB(CallBacker*);
    void		psVwrWinClosedCB(CallBacker*);

    void		viewChgCB(CallBacker*);
    void		lvlChgCB(CallBacker*);
    void		flatChgCB(CallBacker*);
    void		dispEachChgCB(CallBacker*);
    void		curModEdChgCB(CallBacker*);
    void		canvasResizeCB(CallBacker*);
    void		applyReqCB(CallBacker*);
    void		dataMgrClosedCB(CallBacker*)	{ uidatamgr_ = 0; }

    friend class	uiStratSynthDispDSSel;

};
