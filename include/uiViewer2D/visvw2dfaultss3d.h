#ifndef visvw2dfaultss3d_h
#define visvw2dfaultss3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "visvw2ddata.h"

#include "emposid.h"

class CubeSampling;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }


mClass(uiViewer2D) VW2DFaultSS3D : public Vw2DEMDataObject
{
public:
    static VW2DFaultSS3D* create(const EM::ObjectID& id,uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& ed)
			    mCreateVw2DDataObj(VW2DFaultSS3D,id,win,ed);
			~VW2DFaultSS3D();

    void		setCubeSampling(const CubeSampling&, bool upd=false );

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    NotifierAccess*     deSelection()			{ return &deselted_; }

protected:

    void			triggerDeSel();
    void			setEditors();

    MPE::FaultStickSetEditor*   fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<VW2DFaultSS3D>	deselted_;
};

#endif

