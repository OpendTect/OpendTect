#ifndef visvw2dhorizon2d_h
#define visvw2dhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: visvw2dhorizon2d.h,v 1.6 2011/06/03 15:08:41 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace Attrib { class SelSpec; }
namespace FlatView { class AuxDataEditor; }
namespace MPE { class HorizonFlatViewEditor2D; }


mClass Vw2DHorizon2D : public Vw2DEMDataObject
{
public:
    static Vw2DHorizon2D* create(const EM::ObjectID& id,uiFlatViewWin* win,
			       const ObjectSet<uiFlatViewAuxDataEditor>& ed)
				mCreateVw2DDataObj(Vw2DHorizon2D,id,win,ed);

			~Vw2DHorizon2D();

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setLineName(const char*);
    void		setLineSetID( const MultiID& lsetid )
    			{ lsetid_ = lsetid; }

    void		setCubeSampling(const CubeSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );
    
    void		getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor2D>&) const;

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void				triggerDeSel();
    void				setEditors();
    
    const char*				linenm_;
    MultiID				lsetid_;
    const Attrib::SelSpec*		vdselspec_;
    const Attrib::SelSpec*		wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor2D>	horeds_;
    Notifier<Vw2DHorizon2D>		deselted_;
};

#endif
