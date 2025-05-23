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

    void		setTrcKeyZSampling(const TrcKeyZSampling&) override;
    void		setPath(const TrcKeySet&);
    void		setFlatPosData(const FlatPosData*);
    void		drawFault() override;
    void		enableLine(bool);
    void		enableKnots(bool);
    void		setRandomLineID(const RandomLineID&);

protected:

    void			f3dRepaintATSCB(CallBacker*);
    void			f3dRepaintedCB(CallBacker*);

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
					IndexInfo& ix,IndexInfo& iy,
					Coord3& worldpos) const;
    Coord3			getScaleVector() const;
				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z
    Coord3			getNormal(const Coord3* mousepos=nullptr) const;

    const TrcKeySet*		path_;

    EM::Fault3DPainter*		f3dpainter_;
    bool			seedhasmoved_	= false;
    bool			makenewstick_	= false;
    bool			doubleclicked_	= false;
    RandomLineID		rdlid_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_	= mUdf(int);
    MouseEventHandler*		meh_		= nullptr;
    EM::PosID			mousepid_;
};

} // namespace MPE
