
#ifndef mpef3dflatvieweditor_h
#define mpef3dflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2010
 RCS:		$Id: mpef3dflatvieweditor.h,v 1.1 2010-03-16 07:18:07 cvsumesh Exp $
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
    			Fault3DFlatViewEditor(FlatView::AuxDataEditor*);
			~Fault3DFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

    void		setCubeSampling(const CubeSampling&);
    void		drawFault();

protected:
    void			activeF3DChgCB(CallBacker*);

    void			f3dRepaintATSCB(CallBacker*);
    void			f3dRepaintedCB(CallBacker*);

    void 			seedMovementStartedCB(CallBacker*);
    void			seedMovementFinishedCB(CallBacker*);

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);

	mStruct StkMarkerIdInfo
	{
	    int	    merkerid_;
	    int	    stickid_;
	};

    void			cleanActStkContainer();
    void			fillActStkContainer(const EM::ObjectID);

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
