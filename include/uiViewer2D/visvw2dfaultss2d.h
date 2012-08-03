#ifndef visvw2dfaultss2d_h
#define visvw2dfaultss2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dfaultss2d.h,v 1.7 2012-08-03 13:01:17 cvskris Exp $
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "visvw2ddata.h"

#include "emposid.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }


mClass(uiViewer2D) VW2DFaultSS2D : public Vw2DEMDataObject
{
public:
    static VW2DFaultSS2D* create(const EM::ObjectID& id,uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& ed)
    				mCreateVw2DDataObj(VW2DFaultSS2D,id,win,ed);
			~VW2DFaultSS2D();

    void		setLineName(const char*);
    void		setLineSetID( const MultiID& lsetid )
			{ lsetid_ = lsetid; }

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    NotifierAccess*     deSelection()                   { return &deselted_; }

protected:

    void		triggerDeSel();
    void		setEditors();

    const char*			linenm_;
    MultiID			lsetid_;

    MPE::FaultStickSetEditor*	fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<VW2DFaultSS2D>	deselted_;
};

#endif

