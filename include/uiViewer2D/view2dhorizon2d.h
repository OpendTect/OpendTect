#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"
#include "geom2dintersections.h"

#include "emposid.h"

class TrcKeyZSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace Attrib { class SelSpec; }
namespace FlatView { class AuxDataEditor; }
namespace MPE { class HorizonFlatViewEditor2D; }

namespace View2D
{

mExpClass(uiViewer2D) Horizon2D : public EMDataObject
{
mDefStd(Horizon2D)
public:

    void		setSelSpec(const Attrib::SelSpec*,bool wva);
    void		setGeomID(Pos::GeomID);

    void		setTrcKeyZSampling(const TrcKeyZSampling&,
					   bool upd=false);

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    void		setSeedPicking(bool ison);
    void		setTrackerSetupActive(bool ison );
    void		setLine2DInterSectionSet(const Line2DInterSectionSet*
							ln2dintersectionset)
			{ line2dintersectionset_ = ln2dintersectionset; }
    const Line2DInterSectionSet* getLine2DInterSectionSet() const
					    { return line2dintersectionset_; }

    void		getHorEditors(
			  ObjectSet<const MPE::HorizonFlatViewEditor2D>&) const;

    NotifierAccess*	deSelection() override		{ return &deselected_; }

protected:

    void			triggerDeSel() override;
    void			setEditors() override;

    Pos::GeomID			geomid_;
    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    ObjectSet<MPE::HorizonFlatViewEditor2D>	horeds_;
    Notifier<Horizon2D>			deselected_;
    const Line2DInterSectionSet*	line2dintersectionset_;
};

} // namespace View2D
