#ifndef horflatvieweditor_h
#define horflatvieweditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2009
 RCS:           $Id: horflatvieweditor.h,v 1.1 2009-06-22 12:37:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"

class MouseEventHandler;
namespace EM { class Horizon3D; };
namespace FlatView { class AuxDataEditor; }

namespace MPE
{

mClass HorizonFlatViewEditor : public CallBacker
{
public:
    				HorizonFlatViewEditor(FlatView::AuxDataEditor*);
				~HorizonFlatViewEditor();

    void			setCubeSampling(const CubeSampling&);

    FlatView::AuxDataEditor*	getEditor()	{ return editor_; }
    void			setMouseEventHandler(MouseEventHandler*);

protected:

    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);
    void			mouseReleaseCB(CallBacker*);
    void			movementEndCB(CallBacker*);
    void			removePosCB(CallBacker*);

    FlatView::AuxDataEditor*	editor_;
    MouseEventHandler*		mouseeventhandler_;

    CubeSampling		curcs_;

};

} // namespace MPE

#endif

