#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2010
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "emfaultsticksetflatvieweditor.h"
#include "emposid.h"
#include "geometry.h"
#include "integerid.h"

class FlatPosData;
class IndexInfo;
class MouseEventHandler;

namespace EM { class Fault3DPainter; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mExpClass(uiMPE) Fault3DFlatViewEditor : public EM::FaultStickSetFlatViewEditor
{
public:
    			Fault3DFlatViewEditor(FlatView::AuxDataEditor*,
					      const EM::ObjectID&);
			~Fault3DFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setPath(const TrcKeyPath&);
    void		setFlatPosData(const FlatPosData*);
    void		drawFault();
    void		enableLine(bool);
    void		enableKnots(bool);
    void		setRandomLineID(RandomLineID rdlid);

protected:

    void			f3dRepaintATSCB(CallBacker*);
    void			f3dRepaintedCB(CallBacker*);

    void 			seedMovementStartedCB(CallBacker*);
    void			seedMovementFinishedCB(CallBacker*);
    void			removeSelectionCB(CallBacker*);

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);
    void			doubleClickedCB(CallBacker*);
    void			sowingFinishedCB(CallBacker*);

	mStruct(uiMPE) StkMarkerIdInfo
	{
	    int	    markerid_;
	    int	    stickid_;
	};

    void			cleanActStkContainer();
    void			fillActStkContainer();
    int				getStickId(int markerid) const;

    bool			getMousePosInfo(
	    				const Geom::Point2D<int>& mousepos,
					IndexInfo& ix, IndexInfo& iy,
					Coord3& worldpos) const;
    Coord3			getScaleVector() const;
    				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    const TrcKeyPath*		path_;

    EM::Fault3DPainter*		f3dpainter_;
    bool			seedhasmoved_;
    bool			makenewstick_;
    bool			doubleclicked_;
    RandomLineID		rdlid_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_;
    MouseEventHandler*		meh_;
    EM::PosID			mousepid_;
    Coord3			getNormal(const Coord3* mousepos=0) const;
};

}; //namespace MPE

