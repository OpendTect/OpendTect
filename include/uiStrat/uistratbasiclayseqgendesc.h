#ifndef uistratbasiclayseqgendesc_h
#define uistratbasiclayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratbasiclayseqgendesc.h,v 1.4 2012-08-03 13:01:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratextlayseqgendesc.h"
class uiTextItem;
class uiLineItem;
class uiCircleItem;
class uiPolygonItem;
namespace Strat { class LayerGenerator; class SingleLayerGenerator; }


mClass(uiStrat) uiBasicLayerSequenceGenDesc : public uiExtLayerSequenceGenDesc
{
public:

    			uiBasicLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);

    mDefuiExtLayerSequenceGenDescFns(uiBasicLayerSequenceGenDesc,"Basic");

protected:

    struct DispUnit
    {
			DispUnit(uiGraphicsScene&,const Strat::LayerGenerator&);
			~DispUnit();

	uiGraphicsScene& scene_;
	uiTextItem*	nm_;
	uiLineItem*	top_;
	uiCircleItem*	lithcol_;
	uiPolygonItem*	poly_;
	int		topy_;
	int		boty_;

	const Strat::SingleLayerGenerator* gen_;
	bool		genmine_;
    };

    ObjectSet<DispUnit>	disps_;

    void		rebuildDispUnits();
    void		fillDispUnit(int,float,float&);
    void		insertDispUnit(const Strat::LayerGenerator&,int);
    int			curUnitIdx();
    DispUnit*		curUnit();

};


#endif

