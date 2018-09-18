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
class uiMultiFlatViewControl;
class uiStratLayModEditTools;
class uiStratSynthDispDSSel;
class uiStratSynthDataMgr;
class uiTextItem;
class uiWaveletIOObjSel;
namespace FlatView { class AuxData; class Appearance; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:

    typedef Strat::Level::ID			LevelID;
    typedef StratSynth::DataMgr			DataMgr;
    typedef Strat::LayerModel			LayerModel;
    typedef uiStratLayModEditTools		uiEdTools;

			uiStratSynthDisp(uiParent*,DataMgr&,uiEdTools&,uiSize);
			~uiStratSynthDisp();

    DataMgr&		dataMgr()		{ return datamgr_; }
    const DataMgr&	dataMgr() const		{ return datamgr_; }

    void		modelChanged();

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

    uiMultiFlatViewControl* control_;
    ObjectSet<FlatView::AuxData> levelaux_;

    uiFlatViewer*	vwr_;
    uiWaveletIOObjSel*	wvltfld_;
    uiTextItem*		modtypetxtitm_		= 0;
    uiStratSynthDispDSSel* wvaselfld_;
    uiStratSynthDispDSSel* vdselfld_;
    uiStratSynthDataMgr* uidatamgr_		= 0;

    void		setDefaultAppearance(Appearance&);
    void		updFlds();
    void		reDisp();
    void		doDisp(bool wva);
    void		updateViewer(bool wva);
    void		drawLevels();

    int			dispEach() const;
    bool		dispFlattened() const;

    void		initGrp(CallBacker*);
    void		elPropEdCB(CallBacker*);
    void		dataMgrCB(CallBacker*);
    void		expSynthCB(CallBacker*);
    void		wvaSelCB(CallBacker*);
    void		vdSelCB(CallBacker*);
    void		wvltChgCB(CallBacker*);
    void		viewChgCB(CallBacker*);
    void		lvlChgCB(CallBacker*);
    void		flatChgCB(CallBacker*);
    void		curModEdChgCB(CallBacker*);
    void		canvasResizeCB(CallBacker*);
    void		applyReqCB(CallBacker*);
    void		dataMgrClosedCB(CallBacker*)	{ uidatamgr_ = 0; }

    friend class	uiStratSynthDispDSSel;

};
