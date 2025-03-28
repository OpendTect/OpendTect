#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "multiid.h"
#include "position.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace Attrib { class SelSpec; }
namespace EM { class PosID; class EMObject; };
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mExpClass(uiMPE) HorizonFlatViewEditor : public CallBacker
{ mODTextTranslationClass(HorizonFlatViewEditor);
public:
				HorizonFlatViewEditor(FlatView::AuxDataEditor*);
				~HorizonFlatViewEditor();

    void			setTrcKeyZSampling(const TrcKeyZSampling&);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			swapSelSpec();
    void			set2D( bool is2d )	{ is2d_ = is2d; }

    FlatView::AuxDataEditor*	getEditor()	{ return editor_; }
    void			setGeomID( const Pos::GeomID& geomid )
				{ geomid_ = geomid; }
    void			setMouseEventHandler(MouseEventHandler*);
    void			setSeedPickingStatus(bool);
    void			setTrackerSetupActive(bool bn)
				{ trackersetupactive_ = bn; }

    Notifier<HorizonFlatViewEditor> updateoldactivevolinuimpeman;
    Notifier<HorizonFlatViewEditor> restoreactivevolinuimpeman;
    Notifier<HorizonFlatViewEditor> updateseedpickingstatus;

protected:

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);
    void			movementEndCB(CallBacker*);
    void			removePosCB(CallBacker*);

    bool			canTrack(const EMTracker&) const;
    bool			checkSanity(EMTracker&,const EM::EMObject&,
					   const EMSeedPicker&,
					   bool& pickinvd) const;
    bool			prepareTracking(bool pickinvd,
						const EMTracker&,
						EMSeedPicker&,
						const FlatDataPack&) const;
    bool			getPosID(const EM::EMObject&,const Coord3&,
					 EM::PosID&) const;
    bool			doTheSeed(EM::EMObject&,EMSeedPicker&,
					  const Coord3&,
					  const MouseEvent&) const;

    FlatView::AuxDataEditor*	editor_;
    MouseEventHandler*		mouseeventhandler_	= nullptr;

    TrcKeyZSampling		curcs_;
    const Attrib::SelSpec*	vdselspec_		= nullptr;
    const Attrib::SelSpec*	wvaselspec_		= nullptr;

    Pos::GeomID			geomid_;

    bool			is2d_			= false;
    bool			seedpickingon_		= false;
    bool			trackersetupactive_	= false;

public:
    static bool			selectSeedData(const FlatView::AuxDataEditor*,
					       bool& pickinvd);
				/*!<Displays a dlg to select what data to pick
				    on. Returns true if valid result. Result
				    is stored in pickinvd */
};

} // namespace MPE
