#ifndef visvw2dfaultss2d_h
#define visvw2dfaultss2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dfaultss2d.h,v 1.3 2010-11-06 16:21:05 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "emposid.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace MPE { class FaultStickSetFlatViewEditor; class FaultStickSetEditor; }


mClass VW2DFautSS2D : public Vw2DDataObject
{
public:
    			VW2DFautSS2D(const EM::ObjectID&,uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);
			~VW2DFautSS2D();

    void		setLineName(const char*);
    void		setLineSetID( const MultiID& lsetid )
			{ lsetid_ = lsetid; }

    void		draw();
    void		enablePainting(bool yn);
    void		selected(bool enabled=true);

    NotifierAccess*     deSelection()                   { return &deselted_; }

protected:

    void		triggerDeSel();

    uiFlatViewWin*		viewerwin_;

    EM::ObjectID		emid_;
    const char*			linenm_;
    MultiID			lsetid_;

    MPE::FaultStickSetEditor*	fsseditor_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fsseds_;
    Notifier<VW2DFautSS2D>	deselted_;
    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};

#endif
