
#ifndef mpefssflatvieweditor_h
#define mpefssflatvieweditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: mpefssflatvieweditor.h,v 1.1 2010-02-12 08:44:05 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultsticksetflatvieweditor.h"

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

protected:

    void			activeFSSChgCB(CallBacker*);

    void 			seedMovementStartedCB(CallBacker*);
    void			seedMovementFinishedCB(CallBacker*);

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);

    bool			seedhasmoved_;

    FlatView::AuxDataEditor*	editor_;
    int				activeeditorid_;
    int				activestickid_;
    MouseEventHandler*		meh_;
};

} //namespace MPE


#endif
