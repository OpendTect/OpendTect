#ifndef horflatvieweditor2d_h
#define horflatvieweditor2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: horflatvieweditor2d.h,v 1.1 2010-06-24 08:48:33 cvsumesh Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "emposid.h"

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace FlatView { class AuxDataEditor; }
namespace Attrib { class SelSpec; }

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mClass HorizonFlatViewEditor2D : public CallBacker
{
public:
    			HorizonFlatViewEditor2D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor2D();

    void		setCubeSampling(const CubeSampling&);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    
    FlatView::AuxDataEditor* getEditor()		{ return editor_; }
    void		setLineName( const char* lnm )
			{ linenm_ = lnm; }
    void		setLineSetID( const MultiID& lsetid )
			{ lsetid_ = lsetid; }

    void		setMouseEventHandler(MouseEventHandler*);
    void		setSeedPicking(bool);
    void		setTrackerSetupActive(bool yn)
			{ trackersetupactive_ = yn; }

    Notifier<HorizonFlatViewEditor2D> updseedpkingstatus_;

protected:

    void		mouseMoveCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		movementEndCB(CallBacker*);
    void		removePosCB(CallBacker*);

    bool                checkSanity(EMTracker&,const EMSeedPicker&,
	    			    bool& pickinvd) const;
    bool		prepareTracking(bool pickinvd,const EMTracker&,
	    			       EMSeedPicker&,const FlatDataPack&) const;
    bool		getPosID(const Coord3&, EM::PosID&) const;
    bool		doTheSeed(EMSeedPicker&,const Coord3&,
	    			  const MouseEvent&) const;

    EM::ObjectID        	emid_;
    FlatView::AuxDataEditor*	editor_;
    MouseEventHandler*		mehandler_;
    CubeSampling		curcs_;
    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    const char*			linenm_;
    MultiID			lsetid_;

    bool			seedpickingon_;
    bool			trackersetupactive_;    
};

}; // namepace 


#endif
