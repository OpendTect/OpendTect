#ifndef mpefssflatvieweditor_h
#define mpefssflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "uimpemod.h"
#include "emfaultsticksetflatvieweditor.h"

#include "emposid.h"

class FlatPosData;
class MouseEventHandler;
class IndexInfo;

namespace EM { class FaultStickPainter; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mClass(uiMPE) FaultStickSetFlatViewEditor : public EM::FaultStickSetFlatViewEditor
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*,
						    const EM::ObjectID&);
			~FaultStickSetFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setCubeSampling(const CubeSampling&);
    void                setPath(const TypeSet<BinID>*);
    void                setFlatPosData(const FlatPosData*);
    void		drawFault();
    void		enablePainting(bool);
    void		enableKnots(bool);

    void		set2D(bool yn);
    void		setLineName(const char*);
    void		setLineID(const MultiID&);

    TypeSet<int>&	getTrcNos();
    TypeSet<float>&	getDistances();
    TypeSet<Coord>&	getCoords();

protected:

    void			fssRepaintATSCB(CallBacker*);
    void			fssRepaintedCB(CallBacker*);

    void 			seedMovementStartedCB(CallBacker*);
    void			seedMovementFinishedCB(CallBacker*);
    void			removeSelectionCB(CallBacker*);

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);

	mStruct(uiMPE) StkMarkerIdInfo
	{
	    int	    merkerid_;
	    int	    stickid_;
	};

    void			cleanActStkContainer();
    void			fillActStkContainer();
    int				getStickId(int markerid) const; 
    
    bool			getMousePosInfo(
				    const Geom::Point2D<int>& mousepos,
				    IndexInfo& ix, IndexInfo& iy,
				    Coord3& worldpos,int* trcnr=0) const;
    Coord3			getScaleVector() const;
				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    const TypeSet<BinID>*       path_;

    EM::FaultStickPainter*  	fsspainter_;
    bool			seedhasmoved_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_;
    MouseEventHandler*		meh_;
    EM::PosID			mousepid_;
};

} //namespace MPE

#endif


