#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "uimpemod.h"
#include "trckeyzsampling.h"
#include "emposid.h"
#include "flatview.h"
#include "geom2dintersections.h"

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace Attrib { class SelSpec; }
namespace EM { class HorizonPainter2D; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mExpClass(uiMPE) HorizonFlatViewEditor2D : public CallBacker
{ mODTextTranslationClass(HorizonFlatViewEditor2D)
public:
			HorizonFlatViewEditor2D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor2D();

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);

    FlatView::AuxDataEditor* getEditor()		{ return editor_; }
    EM::HorizonPainter2D* getPainter() const		{ return horpainter_; }

    void		setGeomID(Pos::GeomID);
    TypeSet<int>&	getPaintingCanvTrcNos();
    TypeSet<float>&	getPaintingCanDistances();
    void		enableLine(bool);
    void		enableSeed(bool);
    void		enableIntersectionMarker(bool);
    bool		seedEnable() const;
    void		paint();

    void		setMouseEventHandler(MouseEventHandler*);
    void		setSeedPicking(bool);
    void		setTrackerSetupActive( bool yn )
			{ trackersetupactive_ = yn; }
    static bool		selectSeedData(const FlatView::AuxDataEditor* editor,
							      bool& pickinvd);
    void		setLine2DInterSectionSet(const Line2DInterSectionSet*
							ln2dintersectionset)
			{ line2dintersectionset_ = ln2dintersectionset; }
    const Line2DInterSectionSet* getLine2DInterSectionSet()
					    { return line2dintersectionset_; }

    Notifier<HorizonFlatViewEditor2D> updseedpkingstatus_;

protected:

    void		horRepaintATSCB(CallBacker*);
    void		horRepaintedCB(CallBacker*);

    void		mouseMoveCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		movementEndCB(CallBacker*);
    void		removePosCB(CallBacker*);
    void		doubleClickedCB(CallBacker*);

    void		handleMouseClicked(bool dbl);
    bool		checkSanity(EMTracker&,const EMSeedPicker&,
				    bool& pickinvd) const;
    bool		prepareTracking(bool pickinvd,const EMTracker&,
				       EMSeedPicker&,const FlatDataPack&) const;
    bool		getPosID(const Coord3&, EM::PosID&) const;
    bool		doTheSeed(EMSeedPicker&,const Coord3&,
				  const MouseEvent&);
    TrcKey		getTrcKey(const Coord&) const;
    void		setupPatchDisplay();
    void		updatePatchDisplay();
    void		sowingModeCB(CallBacker*);
    void		sowingFinishedCB(CallBacker*);
    void		keyPressedCB(CallBacker*);
    void		polygonFinishedCB(CallBacker*);
    void		releasePolygonSelectionCB(CallBacker*);
    void		preferColorChangedCB(CallBacker*);
    void		undo();
    void		redo();
    EMSeedPicker*	getEMSeedPicker() const;


	mStruct(uiMPE) Hor2DMarkerIdInfo
	{
	    FlatView::AuxData*	marker_;
	    int			markerid_;
	};

    void			cleanAuxInfoContainer();
    void			fillAuxInfoContainer();
    FlatView::AuxData*		getAuxData(int markerid);
    EM::SectionID		getSectionID(int markerid);

    EM::ObjectID		emid_;
    EM::HorizonPainter2D*	horpainter_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<Hor2DMarkerIdInfo> markeridinfos_;
    MouseEventHandler*		mehandler_;
    TrcKeyZSampling		curcs_;
    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    Pos::GeomID			geomid_;

    bool			seedpickingon_;
    bool			trackersetupactive_;
    TrcKey			pickedpos_;
    mutable bool		dodropnext_;
    FlatView::AuxData*		patchdata_;
    TypeSet<EM::PosID>		pointselections_;
    bool			sowingmode_;
    bool			pickinvd_;
    const Line2DInterSectionSet*	line2dintersectionset_;
};

} // namespace MPE
