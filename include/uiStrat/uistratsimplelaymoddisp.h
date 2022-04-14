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


mExpClass(uiStrat) uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{ mODTextTranslationClass(uiStratSimpleLayerModelDisp)
public:

			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelSuite&);
			~uiStratSimpleLayerModelDisp();

    bool		isPerModelDisplay() const override	{ return true; }
    void		handleModelChange();

protected:

    uiTextItem*		emptyitm_ = nullptr;
    ObjectSet<FlatView::AuxData> layerads_;
    ObjectSet<FlatView::AuxData> levelads_;
    FlatView::AuxData*	selseqad_ = nullptr;

    FlatDataPack*	fvdp_;
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
    void		removeLayers(Strat::LayerSequence&,int,bool);
    void		clearObsoleteAuxDatas(ObjectSet<FlatView::AuxData>&,
					      int);
    void		forceRedispAll();

    void		drawSelectedSequence() override;
    void		selPropChg() override;
    void		selLevelChg() override;
    void		selContentChg() override;
    void		dispEachChg() override;
    void		dispLithChg() override;
    void		handleClick(bool dble) override;

};

