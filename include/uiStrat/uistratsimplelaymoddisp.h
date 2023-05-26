#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"

#include "uistratlaymoddisp.h"

#include "datapack.h"


mExpClass(uiStrat) uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{ mODTextTranslationClass(uiStratSimpleLayerModelDisp)
public:

			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelSuite&);
			~uiStratSimpleLayerModelDisp();

    bool		isPerModelDisplay() const override	{ return true; }
    void		handleModelChange() override;

protected:

    uiTextItem*		emptyitm_ = nullptr;
    ObjectSet<FlatView::AuxData> layerads_;
    ObjectSet<FlatView::AuxData> levelads_;
    FlatView::AuxData*	selseqad_ = nullptr;

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

    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
};
