#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratlaymoddisp.h"
#include "stratlevel.h"
class uiLineItem;
class uiTextItem;
class uiRectItem;
class uiFlatViewer;
class uiGraphicsScene;
class uiGraphicsItemSet;
namespace Strat { class LayerSequence; class Content; }


mExpClass(uiStrat) uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{ mODTextTranslationClass(uiStratSimpleLayerModelDisp)
public:

			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelProvider&);
			~uiStratSimpleLayerModelDisp();

    virtual void	modelChanged();
    virtual void	reSetView();
    virtual uiWorldRect	zoomBox() const			{ return zoomwr_; }
    virtual void	setZoomBox(const uiWorldRect&);
    virtual float	getDisplayZSkip() const;

    bool&		fillLayerBoxes()		{ return fillmdls_; }
    bool&		useLithColors()			{ return uselithcols_; }

protected:

    uiTextItem*		emptyitm_;
    uiRectItem*		zoomboxitm_;
    uiGraphicsItemSet&	logblcklineitms_;
    uiGraphicsItemSet&	logblckrectitms_;
    uiGraphicsItemSet&	lvlitms_;
    uiGraphicsItemSet&	contitms_;
    uiLineItem*		selseqitm_;
    ObjectSet<FlatView::AuxData> layerads_;
    ObjectSet<FlatView::AuxData> levelads_;
    FlatView::AuxData*	selseqad_;

    RefMan<FlatDataPack> emptydp_;
    uiWorldRect		zoomwr_;
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    bool		uselithcols_;
    bool		showzoomed_;
    const Strat::Content* selectedcontent_;
    bool		allcontents_;
    Interval<float>	vrg_;

    void		doDraw();
    void		eraseAll();
    void		selPropChgCB(CallBacker*);
    void		selContentChgCB(CallBacker*);
    void		selLevelChgCB(CallBacker*);
    void		dispLithChgCB(CallBacker*);
    void		dispEachChgCB(CallBacker*);
    void		dispZoomedChgCB(CallBacker*);
    void		reDrawAll();
    void		reDrawSeq();
    void		reDrawLevels();
    void		getBounds();
    void		handleClick(bool dble);
    void		handleRightClick(int);
    void		drawLevels(); // deprecated
    virtual void	drawSelectedSequence();
    void		updZoomBox();
    void		updateDataPack();
    void		updateLayerAuxData();
    void		updateLevelAuxData();
    void		updateSelSeqAuxData();
    int			getXPix(int,float) const;
    void		doLayModIO(bool);
    bool		isDisplayedModel(int) const;
    void		removeLayers(Strat::LayerSequence&,int,bool);
    void		forceRedispAll(bool modeledited=false);
    void		doLevelChg();
    int			totalNrLayersToDisplay() const;

};
