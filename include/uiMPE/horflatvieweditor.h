#ifndef horflatvieweditor_h
#define horflatvieweditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2009
 RCS:           $Id: horflatvieweditor.h,v 1.8 2012-08-03 13:01:02 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "callback.h"
#include "cubesampling.h"
#include "multiid.h"
#include "position.h"

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace Attrib { class SelSpec; }
namespace EM { class Horizon3D; class PosID; class EMObject; };
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mClass(uiMPE) HorizonFlatViewEditor : public CallBacker
{
public:
    				HorizonFlatViewEditor(FlatView::AuxDataEditor*);
				~HorizonFlatViewEditor();

    void			setCubeSampling(const CubeSampling&);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			swapSelSpec();
    void			set2D(bool is2d)	{ is2d_ = is2d; }

    FlatView::AuxDataEditor*	getEditor()	{ return editor_; }
    void			setLineName(const char* lnm)							{ linenm_ = lnm; }
    void			setLineSetID(const MultiID& lsetid)
				{ lsetid_ = lsetid; }
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
    MouseEventHandler*		mouseeventhandler_;

    CubeSampling		curcs_;
    const Attrib::SelSpec* 	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    const char*			linenm_;
    MultiID			lsetid_;

    bool			is2d_;
    bool			seedpickingon_;
    bool			trackersetupactive_;
};

} // namespace MPE

#endif


