#ifndef visvw2dfault_h
#define visvw2dfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: visvw2dfault.h,v 1.5 2011/06/03 15:08:41 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class Fault3DFlatViewEditor; class FaultEditor; }


mClass VW2DFault : public Vw2DEMDataObject
{
public:
    static VW2DFault* 	create(const EM::ObjectID& id,uiFlatViewWin* win,
				 const ObjectSet<uiFlatViewAuxDataEditor>& ed)
			     mCreateVw2DDataObj(VW2DFault,id,win,ed);
			~VW2DFault();

    void		setCubeSampling(const CubeSampling&, bool upd=false );
    
    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void		triggerDeSel();
    void		setEditors();

    MPE::FaultEditor*	f3deditor_;
    ObjectSet<MPE::Fault3DFlatViewEditor> faulteds_;
    Notifier<VW2DFault>		deselted_;
};

#endif
