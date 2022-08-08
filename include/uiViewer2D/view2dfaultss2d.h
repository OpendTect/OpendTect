#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

#include "emposid.h"
#include "posgeomid.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }


mExpClass(uiViewer2D) VW2DFaultSS2D : public Vw2DEMDataObject
{
mDefStd(VW2DFaultSS2D)
public:
    void		setGeomID( Pos::GeomID geomid )
			{ geomid_ = geomid; }

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection() override		{ return &deselted_; }

protected:

    void		triggerDeSel() override;
    void		setEditors() override;

    Pos::GeomID		geomid_;
    bool		knotenabled_;

    MPE::FaultStickSetEditor*	fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<VW2DFaultSS2D>	deselted_;

};
