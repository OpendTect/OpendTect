#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"

#include "emposid.h"
#include "posgeomid.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }

namespace View2D
{

mExpClass(uiViewer2D) FaultSS2D : public EMDataObject
{
mDefStd(FaultSS2D)
public:
    void		setGeomID( Pos::GeomID geomid )
			{ geomid_ = geomid; }

    void		draw();
    void		enablePainting(bool yn);
    void		selected();

    NotifierAccess*	deSelection() override		{ return &deselected_; }

protected:

    void		triggerDeSel() override;
    void		setEditors() override;

    Pos::GeomID		geomid_;
    bool		knotenabled_;

    MPE::FaultStickSetEditor*	fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<FaultSS2D>	deselected_;

};

} // namespace View2D
