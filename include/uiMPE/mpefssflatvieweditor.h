#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"

#include "emfaultsticksetflatvieweditor.h"
#include "emposid.h"
#include "geometry.h"

class FlatPosData;
class MouseEventHandler;
class IndexInfo;

namespace EM { class FaultStickPainter; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mExpClass(uiMPE) FaultStickSetFlatViewEditor
			: public EM::FaultStickSetFlatViewEditor
{
public:
			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*,
						    const EM::ObjectID&);
			~FaultStickSetFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setTrcKeyZSampling(const TrcKeyZSampling&) override;
    void		setPath(const TrcKeyPath&);
    void		setFlatPosData(const FlatPosData*);
    void		drawFault() override;
    void		enableLine(bool);
    void		enableKnots(bool);

    void		set2D(bool yn);
    void		setGeomID(const Pos::GeomID&);

    TypeSet<int>&	getTrcNos();
    TypeSet<float>&	getDistances();
    TypeSet<Coord>&	getCoords();
    void		setRandomLineID(const RandomLineID&);

protected:

    void			fssRepaintATSCB(CallBacker*);
    void			fssRepaintedCB(CallBacker*);

    void			seedMovementStartedCB(CallBacker*);
    void			seedMovementFinishedCB(CallBacker*);
    void			removeSelectionCB(CallBacker*);

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);
    void			doubleClickedCB(CallBacker*);
    void			sowingFinishedCB(CallBacker*);

				mStruct(uiMPE) StkMarkerIdInfo
				{
				    int     markerid_;
				    int     stickid_;
				};

    void			cleanActStkContainer();
    void			fillActStkContainer();
    int				getStickId(int markerid) const;

    bool			getMousePosInfo(
				    const Geom::Point2D<int>& mousepos,
				    IndexInfo& ix, IndexInfo& iy,
				    Coord3& worldpos,int* trcnr=nullptr) const;
    Coord3			getScaleVector() const;
				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z
    Coord3			getNormal(const Coord3* mousepos=nullptr) const;

    const TrcKeyPath*		path_		= nullptr;

    EM::FaultStickPainter*	fsspainter_;
    bool			seedhasmoved_	= false;
    bool			makenewstick_	= false;
    bool			doubleclicked_	= false;
    RandomLineID		rdlid_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_	= -1;
    MouseEventHandler*		meh_		= nullptr;
    EM::PosID			mousepid_;
};

} // namespace MPE
