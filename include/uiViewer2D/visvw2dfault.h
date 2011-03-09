#ifndef visvw2dfault_h
#define visvw2dfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: visvw2dfault.h,v 1.3 2011-03-09 07:28:37 cvsranojay Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class Fault3DFlatViewEditor; class FaultEditor; }


mClass VW2DFault : public Vw2DDataObject
{
public:
    			VW2DFault(const EM::ObjectID&,uiFlatViewWin*,
				 const ObjectSet<uiFlatViewAuxDataEditor>&);
			~VW2DFault();

    void		setCubeSampling(const CubeSampling&, bool upd=false );
    
    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void		triggerDeSel();

    uiFlatViewWin*		viewerwin_;
    EM::ObjectID		emid_;

    MPE::FaultEditor*	f3deditor_;
    ObjectSet<MPE::Fault3DFlatViewEditor> faulteds_;
    Notifier<VW2DFault>		deselted_;
    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};

#endif
