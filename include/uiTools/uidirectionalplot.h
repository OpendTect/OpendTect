#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uidirectionalplot.h,v 1.8 2009-04-16 08:50:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "statdirdata.h"
#include "draw.h"
class uiTextItem;
class uiLineItem;
class uiCircleItem;
class uiMarkerItem;
class uiCurvedItem;
class uiGraphicsItem;
class uiGraphicsItemGroup;

/*!\brief creates a directional plot: Rose diagrams and more.

  * For Type Rose, the sector part data's val_ will be interpreted as a count;
    the 'pos_' is ignored.
  * For Type Scatter, (pos_,val_) are the polar coordinates.
  * For Type Vals, the pos_ will be used as the 'R' of a polar coordinate. The
    'val_' will be used for a color table lookup.

 */

mClass uiDirectionalPlot : public uiGraphicsView
{
public:

    mStruct Setup
    {
	enum Type		{ Rose, Scatter, Vals };
				Setup( Type t=Rose )
				    : type_(t)
				    , circlels_(LineStyle::Solid)
				    , sectorls_(LineStyle::Solid)
				    , equils_(LineStyle::Dot)
				    , markstyle_(MarkerStyle2D::Circle)
				    , docount_(false)
				    , curissel_(true)
				    , prefsize_(400,400)	{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(LineStyle,circlels)
	mDefSetupMemb(LineStyle,sectorls)
	mDefSetupMemb(LineStyle,equils)
	mDefSetupMemb(MarkerStyle2D,markstyle)
	mDefSetupMemb(bool,curissel)	// Must clicked sector become selected?
	mDefSetupMemb(bool,docount)	// Show count rather than val_ (Vals)
	mDefSetupMemb(uiSize,prefsize)
    };

				uiDirectionalPlot(uiParent*,const Setup&);
				~uiDirectionalPlot();

    void			setData(const Stats::DirectionalData&);
    void			setData(const float*,int);	//!< Rose dgrm
    Stats::DirectionalData::Setup& dataSetup()	{ return data_.setup_; }

    Setup&			setup()		{ return setup_; }
    const Stats::DirectionalData& data() const	{ return data_; }

    int				nrSectors() const { return data().size(); }
    int				size() const	{ return nrSectors(); }
    float			angle( int s ) const { return data_.angle(s); }

    Notifier<uiDirectionalPlot>	sectorPicked;
    int				curSector() const { return cursector_; }

    void			setSelectedSector( int i )
    						{ selsector_ = i; }
    int				selSector() const;

protected:

    Setup			setup_;
    Stats::DirectionalData	data_;

    bool			isempty_;
    Interval<float>		valrg_;
    Interval<float>		posrg_;
    uiPoint			center_;
    int				radius_;
    int				cursector_;
    int				selsector_;

    uiGraphicsItemGroup&	sectorlines_;
    uiCircleItem*		outercircleitm_;
    uiCurvedItem*		selsectoritem_;
    ObjectSet<uiCircleItem>	equicircles_;
    ObjectSet<uiTextItem>	dirtxtitms_;
    ObjectSet<uiLineItem>	dirlnitms_;
    uiGraphicsItemGroup&	markeritems_;
    uiGraphicsItemGroup&	curveitems_;

    void			mouseRelease(CallBacker*);
    void			reSized(CallBacker*);

    void			gatherInfo();
    void			draw();
    void			drawGrid();
    void			drawAnnot();
    void			drawData();
    void			drawScatter();
    void			drawVals();
    void			drawRose();
    void			drawSelection();

    uiPoint			getUIPos(float r,float usrang) const;

};


#endif
