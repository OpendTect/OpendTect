#ifndef horflatvieweditor_h
#define horflatvieweditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2009
 RCS:           $Id: horflatvieweditor.h,v 1.3 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"
#include "multiid.h"
#include "position.h"

class MouseEventHandler;

namespace Attrib { class SelSpec; }
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
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			swapSelSpec();
    void			set2D(bool is2d)	{ is2d_ = is2d; }

    FlatView::AuxDataEditor*	getEditor()	{ return editor_; }
    void			setLineName(const char* lnm)							{ linenm_ = lnm; }
    void			setLineSetID(const MultiID& lsetid)
				{ lsetid_ = lsetid; }
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
    const Attrib::SelSpec* 	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    const char*			linenm_;
    MultiID			lsetid_;

    bool			is2d_;
};

} // namespace MPE

#endif

