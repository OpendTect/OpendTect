#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratextlayseqgendesc.h"
class uiTextItem;
class uiLineItem;
class uiCircleItem;
class uiPolygonItem;
namespace Strat { class LayerGenerator; class SingleLayerGenerator; }


mExpClass(uiStrat) uiBasicLayerSequenceGenDesc
					: public uiExtLayerSequenceGenDesc
{
public:

			uiBasicLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);
			~uiBasicLayerSequenceGenDesc();

    mDefuiExtLayerSequenceGenDescFns(uiBasicLayerSequenceGenDesc,"Basic");

protected:

    struct DispUnit
    {
			DispUnit(uiGraphicsScene&,const Strat::LayerGenerator&);
			~DispUnit();

	uiGraphicsScene& scene_;
	uiTextItem*	nm_ = nullptr;
	uiLineItem*	top_;
	uiCircleItem*	lithcol_;
	uiPolygonItem*	poly_;
	int		topy_;
	int		boty_;

	const Strat::SingleLayerGenerator* gen_;
	bool		genmine_ = false;
    };

    ObjectSet<DispUnit>	disps_;

    void		rebuildDispUnits();
    void		fillDispUnit(int,float,float&);
    void		insertDispUnit(const Strat::LayerGenerator&,int);
    int			curUnitIdx();
    DispUnit*		curUnit();

};
