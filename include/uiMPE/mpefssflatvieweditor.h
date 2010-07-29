#ifndef mpefssflatvieweditor_h
#define mpefssflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:           $Id: mpefssflatvieweditor.h,v 1.7 2010-07-29 12:02:32 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "emposid.h"

class MouseEventHandler;

namespace EM { class FaultStickPainter; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mClass FaultStickSetFlatViewEditor : public EM::FaultStickSetFlatViewEditor
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*,
						    const EM::ObjectID&);
			~FaultStickSetFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setCubeSampling(const CubeSampling&);
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

	mStruct StkMarkerIdInfo
	{
	    int	    merkerid_;
	    int	    stickid_;
	};

    void			cleanActStkContainer();
    void			fillActStkContainer();
    const int			getStickId(int markerid) const; 

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
