
#ifndef mpefssflatvieweditor_h
#define mpefssflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:           $Id: mpefssflatvieweditor.h,v 1.2 2010-03-02 06:51:06 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

#include "emposid.h"

class MouseEventHandler;

namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mClass FaultStickSetFlatViewEditor : public EM::FaultStickSetFlatViewEditor
{
public:
    			FaultStickSetFlatViewEditor(FlatView::AuxDataEditor*);
			~FaultStickSetFlatViewEditor();

    void		setMouseEventHandler(MouseEventHandler*);
    void		updateActStkContainer();

protected:

    void			activeFSSChgCB(CallBacker*);

    void			fssRepaintATSCB(CallBacker*);
    void			fssRepaintedCB(CallBacker*);

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

    bool			seedhasmoved_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<StkMarkerIdInfo>	markeridinfo_;
    int				activestickid_;
    MouseEventHandler*		meh_;
    EM::PosID			mousepid_;
};

} //namespace MPE


#endif
