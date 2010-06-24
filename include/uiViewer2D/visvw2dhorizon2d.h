#ifndef visvw2dhorizon2d_h
#define visvw2dhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: visvw2dhorizon2d.h,v 1.1 2010-06-24 08:37:17 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace Attrib { class SelSpec; }
namespace FlatView { class AuxDataEditor; }
namespace EM { class HorizonPainter2D; }
namespace MPE { class HorizonFlatViewEditor2D; }


mClass Vw2DHorizon2D : public Vw2DDataObject
{
public:
    			Vw2DHorizon2D(const EM::ObjectID&,uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);
			~Vw2DHorizon2D();

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setLineName(const char*);
    void		setLineSetID( const MultiID& lsetid )
    			{ lsetid_ = lsetid; }

    void		setCubeSampling(const CubeSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void				triggerDeSel();
    
    uiFlatViewWin*			viewerwin_;			

    EM::ObjectID        		emid_;
    const char*				linenm_;
    MultiID				lsetid_;
    const Attrib::SelSpec*		vdselspec_;
    const Attrib::SelSpec*		wvaselspec_;

    ObjectSet<EM::HorizonPainter2D>	horpainters_;
    ObjectSet<MPE::HorizonFlatViewEditor2D>	horeds_;
    Notifier<Vw2DHorizon2D>		deselted_;
    const ObjectSet<uiFlatViewAuxDataEditor>&	auxdataeditors_;
};

#endif
