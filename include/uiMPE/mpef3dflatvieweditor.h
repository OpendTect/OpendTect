
#ifndef mpef3dflatvieweditor_h
#define mpef3dflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2010
 RCS:		$Id: mpef3dflatvieweditor.h,v 1.3 2010-07-27 09:58:50 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "emposid.h"

class MouseEventHandler;

namespace EM { class Fault3DPainter; }
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mClass Fault3DFlatViewEditor : public EM::FaultStickSetFlatViewEditor
{
public:
    			Fault3DFlatViewEditor(FlatView::AuxDataEditor*,
					      const EM::ObjectID&);
			~Fault3DFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setCubeSampling(const CubeSampling&);
    void		drawFault();
    void		enablePainting(bool);
    void		enableKnots(bool);

protected:

    void			f3dRepaintATSCB(CallBacker*);
    void			f3dRepaintedCB(CallBacker*);

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

    EM::Fault3DPainter*		f3dpainter_;
    bool			seedhasmoved_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_;
    MouseEventHandler*		meh_;
    EM::PosID			mousepid_;
};

}; //namespace MPE

#endif
