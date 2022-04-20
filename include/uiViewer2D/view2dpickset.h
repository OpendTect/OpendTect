#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "flatview.h"
#include "flatauxdataeditor.h"
#include "trckeyzsampling.h"
#include "view2ddata.h"

class uiFlatViewer;
class uiFlatViewAuxDataEditor;
namespace Pick { class Set; }


mExpClass(uiViewer2D) VW2DPickSet : public Vw2DDataObject
{
public:
    static VW2DPickSet* create(int id,uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& ed)
			    mCreateVw2DDataObj(VW2DPickSet,id,win,ed);
			~VW2DPickSet();

    void		drawAll();
    void		clearPicks();
    void		enablePainting(bool yn);
    void		selected();
    MultiID		pickSetID() const;

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		pickAddChgCB(CallBacker*);
    void		pickRemoveCB(CallBacker*);
    void		dataChangedCB(CallBacker*);
    MarkerStyle2D	get2DMarkers(const Pick::Set& ps) const;
    void		triggerDeSel();

    RefMan<Pick::Set>		pickset_;
    bool			isselected_;
    Notifier<VW2DPickSet>	deselected_;
    TypeSet<int>		auxids_;

    ObjectSet<FlatView::AuxData>	picks_;
    ObjectSet<uiFlatViewAuxDataEditor>	editors_;
    ObjectSet<uiFlatViewer>		viewers_;
};

