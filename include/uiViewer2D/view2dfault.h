#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

#include "emposid.h"

class TrcKeyZSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class Fault3DFlatViewEditor; class FaultEditor; }


mExpClass(uiViewer2D) VW2DFault : public Vw2DEMDataObject
{
mDefStd(VW2DFault)
public:

    void		setTrcKeyZSampling(
				const TrcKeyZSampling&,bool upd=false);

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void		triggerDeSel();
    void		setEditors();

    bool		knotenabled_;
    MPE::FaultEditor*	f3deditor_;
    ObjectSet<MPE::Fault3DFlatViewEditor> faulteds_;
    Notifier<VW2DFault>		deselted_;
};
