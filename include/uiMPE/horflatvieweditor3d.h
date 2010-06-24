#ifndef horflatvieweditor3d_h
#define horflatvieweditor3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: horflatvieweditor3d.h,v 1.1 2010-06-24 08:48:33 cvsumesh Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "emposid.h"

namespace FlatView { class AuxDataEditor; }
namespace Attrib { class SelSpec; }

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mClass HorizonFlatViewEditor3D : public CallBacker
{
public:
    			HorizonFlatViewEditor3D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor3D();
    
    void		setCubeSampling(const CubeSampling&);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);

    FlatView::AuxDataEditor* getEditor()                { return editor_; }
    void		setMouseEventHandler(MouseEventHandler*);

    void		setSeedPicking(bool);
    void		setTrackerSetupActive(bool yn)
			{ trackersetupactive_ = yn; }

    Notifier<HorizonFlatViewEditor3D>	updseedpkingstatus_;

protected:

    void		mouseMoveCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		movementEndCB(CallBacker*);
    void		removePosCB(CallBacker*);

    bool		checkSanity(EMTracker&,const EMSeedPicker&,
	    			    bool& pickinvd) const;
    bool		prepareTracking(bool pickinvd,const EMTracker&,
	    			       EMSeedPicker&,const FlatDataPack&) const;
    bool		getPosID(const Coord3&,EM::PosID&) const;
    bool		doTheSeed(EMSeedPicker&,const Coord3&,
	    			  const MouseEvent&) const;

    EM::ObjectID		emid_;
    FlatView::AuxDataEditor*	editor_;
    MouseEventHandler*		mehandler_;
    CubeSampling		curcs_;
    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    bool			seedpickingon_;
    bool			trackersetupactive_;
};

} //namespace MPE


#endif
