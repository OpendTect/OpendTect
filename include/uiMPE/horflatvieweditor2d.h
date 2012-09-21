#ifndef horflatvieweditor2d_h
#define horflatvieweditor2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "uimpemod.h"
#include "cubesampling.h"
#include "emposid.h"
#include "flatview.h"

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

mClass(uiMPE) HorizonFlatViewEditor2D : public CallBacker
{
public:
    			HorizonFlatViewEditor2D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor2D();

    void		setCubeSampling(const CubeSampling&);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    
    FlatView::AuxDataEditor* getEditor()		{ return editor_; }
    EM::HorizonPainter2D* getPainter() const 		{ return horpainter_; }

    void		setLineName(const char*);
    void		setLineSetID( const MultiID& lsetid )
			{ lsetid_ = lsetid; }

    TypeSet<int>&	getPaintingCanvTrcNos();
    TypeSet<float>&	getPaintingCanDistances();
    void		enableLine(bool);
    void		enableSeed(bool);
    void		paint();

    void		setMouseEventHandler(MouseEventHandler*);
    void		setSeedPicking(bool);
    void		setTrackerSetupActive(bool yn)
			{ trackersetupactive_ = yn; }


    Notifier<HorizonFlatViewEditor2D> updseedpkingstatus_;

protected:

    void		horRepaintATSCB(CallBacker*);
    void		horRepaintedCB(CallBacker*);

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

    	mStruct(uiMPE) Hor2DMarkerIdInfo
	{
	    FlatView::AuxData*	marker_;
	    int			merkerid_;
	    EM::SectionID	sectionid_;
	};

    void			cleanAuxInfoContainer();
    void			fillAuxInfoContainer();
    FlatView::AuxData*		getAuxData(int markerid);
    EM::SectionID		getSectionID(int markerid);

    EM::ObjectID        	emid_;
    EM::HorizonPainter2D*	horpainter_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<Hor2DMarkerIdInfo> markeridinfos_;
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


