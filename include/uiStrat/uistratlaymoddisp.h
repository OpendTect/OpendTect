#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
________________________________________________________________________

-*/

#include "flatview.h"
#include "uistratmod.h"
#include "uigroup.h"
#include "uistring.h"
#include "coltabsequence.h"
#include "coltabmapper.h"

class PropertyRef;
class uiGraphicsScene;
class uiStratLayModEditTools;
class uiTextItem;
class uiFlatViewer;
class UnitOfMeasure;
namespace Strat {
    class Layer; class LayerModel; class LayerModelSuite;
    class LayerSequence; class Level;
}


/*!\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nequences+1] vs zrg_.
*/


mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{ mODTextTranslationClass(uiStratLayerModelDisp);
public:

    typedef TypeSet<float>		LVLZVals;
    typedef TypeSet< LVLZVals >		LVLZValsSet;
    typedef Strat::Layer		Layer;
    typedef Strat::LayerModel		LayerModel;
    typedef Strat::LayerModelSuite	LayerModelSuite;
    typedef Strat::LayerSequence	LayerSequence;
    typedef Strat::Level		Level;

			uiStratLayerModelDisp(uiStratLayModEditTools&,
					    const LayerModelSuite&);
			~uiStratLayerModelDisp();
    virtual bool	isPerModelDisplay() const	{ return false; }

    virtual void	handleModelChange()		= 0;
    uiWorldRect		zoomBox() const;
    void		setZoomBox(const uiWorldRect&);
    void		clearZoom();
    uiFlatViewer*	getViewerClone(uiParent*) const;

    uiSize		initialSize() const { return uiSize(600,350); }
    const LayerModel&	layerModel() const;
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);
    int			selLevelIdx() const;
    const LVLZValsSet&	getLevelDepths()		{ return lvldpths_; }
    float		getLayerPropValue(const Layer&,const UnitOfMeasure*,
							int) const;
    float		getLayerPropValue(const Layer&,const PropertyRef&,
							int) const;

    uiFlatViewer*	getViewer()			{ return &vwr_; }

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> modelChanged;
    Notifier<uiStratLayerModelDisp> sequencesAdded; // precedes modelChanged
    CNotifier<uiStratLayerModelDisp,const uiString*> infoChanged;

protected:

    const LayerModelSuite& lms_;
    const uiStratLayModEditTools& tools_;

    uiFlatViewer&	vwr_;
    const bool		zinfeet_;
    LVLZValsSet		lvldpths_;
    IOPar		dumppars_;
    uiTextItem*		modtypetxtitm_			= 0;
    int			selseqidx_			= -1;

    uiGraphicsScene&	scene() const;

    bool		showFlattened() const;
    int			curPropIdx() const;
    int			usrPointedModelNr() const;
    Layer*		usrPointedLayer(int&) const;
    bool		doLayerModelIO(bool);
    Interval<float>	getModelRange(int propidx) const;
    void		getFlattenedZRange(Interval<float>&) const;
    void		fillLevelDepths();

    void		initGrp(CallBacker*);
    void		vwResizeCB(CallBacker*);
    void		mouseMovedCB(CallBacker*);
    void		doubleClickedCB(CallBacker*);
    void		usrClickedCB(CallBacker*);
    void		curModEdChgCB(CallBacker*);
    void		showFlatChgCB(CallBacker*)	{ showFlatChg(); }
    void		selPropChgCB(CallBacker*)	{ selPropChg(); }
    void		dispLithChgCB(CallBacker*)	{ dispLithChg(); }
    void		selContentChgCB(CallBacker*)	{ selContentChg(); }
    void		selLevelChgCB(CallBacker*)	{ selLevelChg(); }
    void		dispEachChgCB(CallBacker*)	{ dispEachChg(); }
    void		dispZoomedChgCB(CallBacker*)	{ dispZoomedChg(); }

    virtual void	drawSelectedSequence()		= 0;
    virtual void	selPropChg()			= 0;
    virtual void	selContentChg()			= 0;
    virtual void	selLevelChg()			= 0;
    virtual void	handleClick(bool dble)		= 0;

    virtual void	dispZoomedChg();
    virtual void	dispLithChg()			{}
    virtual void	dispEachChg()			{}
    virtual void	showFlatChg();

};
