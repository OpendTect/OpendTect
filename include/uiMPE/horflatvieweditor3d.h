#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "emposid.h"
#include "flatview.h"
#include "trckeyzsampling.h"

namespace Attrib { class SelSpec; }
namespace EM { class HorizonPainter3D; }
namespace FlatView { class AuxDataEditor; }

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mExpClass(uiMPE) HorizonFlatViewEditor3D : public CallBacker
{ mODTextTranslationClass(HorizonFlatViewEditor3D);
public:
			HorizonFlatViewEditor3D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor3D();

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setPath(const TrcKeySet&);
    void		setFlatPosData(const FlatPosData*);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);

    FlatView::AuxDataEditor* getEditor()		{ return editor_; }
    EM::HorizonPainter3D* getPainter() const		{ return horpainter_; }
    void		setMouseEventHandler(MouseEventHandler*);

    void		enableLine(bool);
    void		enableSeed(bool);
    void		paint();
    bool		seedEnable() const;

    void		setSeedPicking( bool yn )	{}
    void		setTrackerSetupActive(bool yn)
			{ trackersetupactive_ = yn; }

    Notifier<HorizonFlatViewEditor3D>	updseedpkingstatus_;

protected:

    void		horRepaintATSCB( CallBacker* );
    void		horRepaintedCB( CallBacker* );

    void		mouseMoveCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		doubleClickedCB(CallBacker*);
    void		movementEndCB(CallBacker*);
    void		removePosCB(CallBacker*);
    void		makePatchEnd(bool);

    void		handleMouseClicked(bool dbl);
    bool		checkSanity(EMTracker&,const EMSeedPicker&,
				    bool& pickinvd) const;
    bool		prepareTracking(bool pickinvd,const EMTracker&,
				       EMSeedPicker&,const FlatDataPack&) const;
    bool		getPosID(const Coord3&,EM::PosID&) const;
    bool		doTheSeed(EMSeedPicker&,const Coord3&,
				  const MouseEvent&);
    void		sowingFinishedCB(CallBacker*);
    void		keyPressedCB(CallBacker*);
    void		polygonFinishedCB(CallBacker*);
    void		undo();
    void		redo();
    EMSeedPicker*	getEMSeedPicker() const;

	mStruct(uiMPE) Hor3DMarkerIdInfo
	{
	    FlatView::AuxData*	marker_;
	    int			markerid_;
	};

    void			cleanAuxInfoContainer();
    void			fillAuxInfoContainer();
    FlatView::AuxData*		getAuxData(int markerid);
    EM::SectionID		getSectionID(int markerid);
    void			setupPatchDisplay();
    void			updatePatchDisplay();
    void			sowingModeCB(CallBacker*);
    void			releasePolygonSelectionCB(CallBacker*);
    void			preferColorChangedCB(CallBacker*);

    EM::ObjectID		emid_;
    EM::HorizonPainter3D*	horpainter_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<Hor3DMarkerIdInfo> markeridinfos_;
    MouseEventHandler*		mehandler_		= nullptr;
    TrcKeyZSampling		curcs_;
    const TrcKeySet*		curtkpath_		= nullptr;
    const Attrib::SelSpec*	vdselspec_		= nullptr;
    const Attrib::SelSpec*	wvaselspec_		= nullptr;

    bool			trackersetupactive_	= false;
    TrcKey			pickedpos_;
    mutable bool		dodropnext_		= false;

    FlatView::AuxData*		patchdata_		= nullptr;
    TypeSet<EM::PosID>		pointselections_;
    bool			sowingmode_		= false;
    bool			pickinvd_		= true;
};

} // namespace MPE
