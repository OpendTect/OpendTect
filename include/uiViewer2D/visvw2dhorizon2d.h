#ifndef visvw2dhorizon2d_h
#define visvw2dhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: visvw2dhorizon2d.h,v 1.4 2010-09-15 08:30:21 cvsbruno Exp $
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
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );
    
    const EM::ObjectID& emID() const 			{ return emid_; }

    void		getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor2D>&) const;

    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void				triggerDeSel();
    
    uiFlatViewWin*			viewerwin_;			

    EM::ObjectID        		emid_;
    const char*				linenm_;
    MultiID				lsetid_;
    const Attrib::SelSpec*		vdselspec_;
    const Attrib::SelSpec*		wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor2D>	horeds_;
    Notifier<Vw2DHorizon2D>		deselted_;
    const ObjectSet<uiFlatViewAuxDataEditor>&	auxdataeditors_;
};

#endif
