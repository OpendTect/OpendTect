#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

class TrcKeyZSampling;

namespace Attrib { class SelSpec; }
namespace MPE { class HorizonFlatViewEditor3D; }


mExpClass(uiViewer2D) Vw2DHorizon3D : public Vw2DEMDataObject
{
public:
   static Vw2DHorizon3D* create(const EM::ObjectID& id,uiFlatViewWin* win,
			   const ObjectSet<uiFlatViewAuxDataEditor>& ed)
			    mCreateVw2DDataObj(Vw2DHorizon3D,id,win,ed);

			~Vw2DHorizon3D();

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setTrcKeyZSampling(
				const TrcKeyZSampling&,bool upd=false);

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );

    void                getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor3D>&) const;


    NotifierAccess*	deSelection()			{ return &deselted_; }

protected:

    void				triggerDeSel();
    void				setEditors();

    void				checkCB(CallBacker*);
    void				deSelCB(CallBacker*);

    const Attrib::SelSpec*		vdselspec_;
    const Attrib::SelSpec*		wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor3D>     horeds_;
    Notifier<Vw2DHorizon3D>		deselted_;
};


