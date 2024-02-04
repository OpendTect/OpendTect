#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"

#include "flatview.h"
#include "flatauxdataeditor.h"
#include "trckeyzsampling.h"
#include "view2ddata.h"
#include "pickset.h"

class uiFlatViewer;
class uiFlatViewAuxDataEditor;

namespace View2D
{

mExpClass(uiViewer2D) PickSet : public DataObject
{
mDefStd(PickSet)
public:

    void		setPickSet(Pick::Set&);

    void		drawAll();
    void		clearPicks();
    void		enablePainting(bool yn);
    void		selected();
    MultiID		pickSetID() const;

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:

    void		pickAddChgCB(CallBacker*);
    void		pickRemoveCB(CallBacker*);
    void		dataChangedCB(CallBacker*);
    MarkerStyle2D	get2DMarkers(const Pick::Set& ps) const;
    void		triggerDeSel() override;

    RefMan<Pick::Set>	pickset_;
    bool		isselected_		= false;
    Notifier<PickSet>	deselected_;
    TypeSet<int>	auxids_;

    ObjectSet<FlatView::AuxData>	picks_;
    ObjectSet<uiFlatViewAuxDataEditor>	editors_;
    ObjectSet<uiFlatViewer>		viewers_;
};

} // namespace View2D
