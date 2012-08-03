#ifndef visvw2dpickset_h
#define visvw2dpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id: visvw2dpickset.h,v 1.6 2012-08-03 13:01:17 cvskris Exp $
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "multiid.h"
#include "flatview.h"
#include "flatauxdataeditor.h"

#include "visvw2ddata.h"

class uiFlatViewer;
class uiFlatViewAuxDataEditor;
class CubeSampling;
namespace Pick { class Set; }


mClass(uiViewer2D) VW2DPickSet : public Vw2DDataObject
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
    const MultiID	pickSetID() const;

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    Coord3		getCoord(const FlatView::Point&) const;
    void		pickAddChgCB(CallBacker*);
    void		pickRemoveCB(CallBacker*);
    void		dataChangedCB(CallBacker*);
    MarkerStyle2D	get2DMarkers(const Pick::Set& ps) const;
    void		triggerDeSel();
    void		updateSetIdx(const CubeSampling&);

    Pick::Set*			pickset_;
    FlatView::AuxData*  	picks_;
    uiFlatViewAuxDataEditor*	editor_;
    uiFlatViewer&		viewer_;
    int				auxid_;
    bool			isselected_;
    Notifier<VW2DPickSet>	deselected_;
    bool			isownremove_;
    TypeSet<int>		picksetidxs_;
};

#endif

