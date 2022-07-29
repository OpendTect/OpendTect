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

class TrcKeyZSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }


mExpClass(uiViewer2D) VW2DFaultSS3D : public Vw2DEMDataObject
{
mDefStd(VW2DFaultSS3D)
public:

    void		setTrcKeyZSampling(
				const TrcKeyZSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void		triggerDeSel();
    void		setEditors();

    bool		knotenabled_;

    MPE::FaultStickSetEditor*   fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<VW2DFaultSS3D>	deselted_;

};
