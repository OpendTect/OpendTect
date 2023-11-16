#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"

#include "flatview.h"
#include "uigroup.h"
#include "uistring.h"

class PropertyRef;
class uiGraphicsScene;
class uiStratLayModEditTools;
class uiTextItem;
class uiFlatViewer;
namespace Strat {
    class Layer; class LayerSequence; class LayerModel;
    class LayerModelSuite; class Level;
}


/*!\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nsequences+1] vs zrg_.
*/


mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{ mODTextTranslationClass(uiStratLayerModelDisp)
public:
			~uiStratLayerModelDisp();

    virtual bool	isPerModelDisplay() const	{ return false; }

    virtual void	handleModelChange()		= 0;
    uiWorldRect		zoomBox() const;
    void		setZoomBox(const uiWorldRect&);
    void		clearZoom();
    uiFlatViewer*	getViewerClone(uiParent*) const;

    uiSize		initialSize() const	{ return uiSize(600,350); }
    const Strat::LayerModel& layerModel() const;
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);
    int			selLevelIdx() const;
    const TypeSet<TypeSet<float> >& getLevelDepths()	{ return lvldpths_; }
    float		getLayerPropValue(const Strat::Layer&,
					  const PropertyRef&,int) const;

    uiFlatViewer*	getViewer()			{ return &vwr_; }

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> sequencesAdded; // precedes modelChanged
    CNotifier<uiStratLayerModelDisp,const uiString*> infoChanged;

protected:
				uiStratLayerModelDisp(uiStratLayModEditTools&,
						const Strat::LayerModelSuite&);

    const Strat::LayerModelSuite& lms_;
    const uiStratLayModEditTools& tools_;

    uiFlatViewer&	vwr_;
    BufferString	depthlbl_;
    TypeSet<TypeSet<float> > lvldpths_;
    IOPar		dumppars_;
    uiTextItem*		modtypetxtitm_			= nullptr;
    int			selseqidx_			= -1;

    uiGraphicsScene&	scene() const;

    bool		showFlattened() const;
    int			curPropIdx() const;
    int			usrPointedModelNr() const;
    Strat::Layer*	usrPointedLayer(int&) const;
    bool		doLayerModelIO(bool);
    Interval<float>	getModelRange(int propidx) const;
    void		getFlattenedZRange(Interval<float>&) const;
    void		fillLevelDepths();
    void		notifyModelChanged(int ctyp);

    void		initGrp(CallBacker*);
    void		vwResizeCB(CallBacker*);
    void		zoomChgCB(CallBacker*);
    void		mouseMovedCB(CallBacker*);
    void		doubleClickedCB(CallBacker*);
    void		usrClickedCB(CallBacker*);
    void		modelChangedCB(CallBacker*);
    void		curModEdChgCB(CallBacker*);
    void		selPropChgCB( CallBacker* )	{ selPropChg(); }
    void		selLevelChgCB( CallBacker* )	{ selLevelChg(); }
    void		selContentChgCB( CallBacker* )	{ selContentChg(); }
    void		dispEachChgCB( CallBacker* )	{ dispEachChg(); }
    void		dispLithChgCB( CallBacker* )	{ dispLithChg(); }
    void		dispZoomedChgCB( CallBacker* )	{ dispZoomedChg(); }
    void		showFlatChgCB( CallBacker* )	{ showFlatChg(); }

    virtual void	drawSelectedSequence()		= 0;
    virtual void	selPropChg()			= 0;
    virtual void	selLevelChg()			= 0;
    virtual void	selContentChg()			= 0;
    virtual void	handleClick(bool dble)		= 0;

    virtual void	dispEachChg()			{}
    virtual void	dispLithChg()			{}
    virtual void	dispZoomedChg();
    virtual void	showFlatChg();

};
