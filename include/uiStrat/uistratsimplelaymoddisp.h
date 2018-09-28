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
class uiTextItem;
class uiFlatViewer;
class uiGraphicsScene;
namespace Strat { class Content; }


mExpClass(uiStrat) uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{ mODTextTranslationClass(uiStratSimpleLayerModelDisp)
public:

			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
						    const LayerModelSuite&);
			~uiStratSimpleLayerModelDisp();

    virtual bool	isPerModelDisplay() const	{ return true; }
    virtual void	handleModelChange();

protected:

    typedef FlatView::AuxData	AuxData;
    typedef ObjectSet<AuxData>	AuxDataSet;

    uiTextItem*		emptyitm_;
    AuxDataSet		layerads_;
    AuxDataSet		levelads_;
    AuxData*		selseqad_;

    RefMan<FlatDataPack> fvdp_;
    Interval<float>	zrg_;
    Interval<float>	curproprg_;

    void		reDrawAll();
    void		reDrawSeqs();
    void		reDrawLevels();
    void		updateDataPack();
    void		handleRightClick(int);
    void		updateLayerAuxData();
    void		updateLevelAuxData();
    void		updateSelSeqAuxData();
    void		doLayModIO(bool);
    void		removeLayers(LayerSequence&,int,bool);
    void		clearObsoleteAuxDatas(AuxDataSet&,int);
    void		forceRedispAll(bool modeledited=false);

    virtual void	drawSelectedSequence();
    virtual void	selPropChg();
    virtual void	handleClick(bool dble);
    virtual void	selContentChg();
    virtual void	selLevelChg();
    virtual void	dispLithChg();
    virtual void	dispEachChg();

};
