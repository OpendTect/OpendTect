#ifndef visvw2dhorizon3d_h
#define visvw2dhorizon3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: visvw2dhorizon3d.h,v 1.4 2010-09-15 08:30:21 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace Attrib { class SelSpec; }
namespace MPE { class HorizonFlatViewEditor3D; }


mClass Vw2DHorizon3D : public Vw2DDataObject
{
public:
    			Vw2DHorizon3D(const EM::ObjectID&,uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);
			~Vw2DHorizon3D();

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setCubeSampling(const CubeSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );
    
    const EM::ObjectID& emID() const 			{ return emid_; }
    void                getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor3D>&) const;


    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void				triggerDeSel();

    void				checkCB(CallBacker*);
    void				deSelCB(CallBacker*);

    uiFlatViewWin*			viewerwin_;

    EM::ObjectID                        emid_;
    const Attrib::SelSpec*		vdselspec_;
    const Attrib::SelSpec*		wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor3D>     horeds_;
    Notifier<Vw2DHorizon3D>		deselted_;
    const ObjectSet<uiFlatViewAuxDataEditor>&	auxdataeditors_;
};


#endif
