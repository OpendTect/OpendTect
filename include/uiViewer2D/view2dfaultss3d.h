#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

#include "emposid.h"

class TrcKeyZSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }

namespace View2D
{

mExpClass(uiViewer2D) FaultSS3D : public EMDataObject
{
mDefStd(FaultSS3D)
public:

    void		setTrcKeyZSampling(
				const TrcKeyZSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection() override		{ return &deselected_; }

protected:

    void		triggerDeSel() override;
    void		setEditors() override;

    bool		knotenabled_;

    MPE::FaultStickSetEditor*   fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<FaultSS3D>	deselected_;

};

} // namespace View2D
