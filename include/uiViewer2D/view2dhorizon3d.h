#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

class TrcKeyZSampling;

namespace Attrib { class SelSpec; }
namespace MPE { class HorizonFlatViewEditor3D; }

namespace View2D
{

mExpClass(uiViewer2D) Horizon3D : public EMDataObject
{
mDefStd(Horizon3D)
public:

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setTrcKeyZSampling(
				const TrcKeyZSampling&,bool upd=false);

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );

    void		getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor3D>&) const;

    NotifierAccess*	deSelection() override		{ return &deselected_; }

protected:

    void			triggerDeSel() override;
    void			setEditors() override;

    void			checkCB(CallBacker*);
    void			deSelCB(CallBacker*);

    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor3D>     horeds_;
    Notifier<Horizon3D>		deselected_;

};

} // namespace View2D
