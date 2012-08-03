#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uidirectionalplot.h,v 1.18 2012-08-03 13:01:12 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uitoolsmod.h"
#include "uigraphicsview.h"
#include "statdirdata.h"
#include "draw.h"

class uiTextItem;
class uiLineItem;
class uiCircleItem;
class uiColTabItem;
class uiCurvedItem;
class uiMarkerItem;
class uiGraphicsItem;
class uiGraphicsItemGroup;
namespace ColTab { class Sequence; }

/*!\brief creates a directional plot: Rose diagrams and more.

  * For Type Rose, the sector part count_ will be used only
  * For Type Scatter, (pos_,val_) are the polar coordinates.
  * For Type Vals, the pos_ will be used as the 'R' of a polar coordinate. The
    'val_' will be used for a color table lookup. If setup_.docount_ the
    coloring will be on count_.

 */

mClass(uiTools) uiDirectionalPlot : public uiGraphicsView
{
public:

    mStruct(uiTools) Setup
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
				    , valcolor_(true)
				    , clipratio_(0.02)
				    , prefsize_(400,400)	{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(LineStyle,circlels)
	mDefSetupMemb(LineStyle,sectorls)
	mDefSetupMemb(LineStyle,equils)
	mDefSetupMemb(MarkerStyle2D,markstyle)
	mDefSetupMemb(bool,curissel)	// Must clicked sector become selected?
	mDefSetupMemb(bool,docount)	// Show count rather than val_ (Vals)
	mDefSetupMemb(bool,valcolor)	// Use val_ to color (Rose)
	mDefSetupMemb(float,clipratio)	// Vals
	mDefSetupMemb(BufferString,hdrannot)
	mDefSetupMemb(BufferString,nameforpos)
	mDefSetupMemb(BufferString,nameforval)
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
    int				selSector() const { return selsector_; }
    void			setSelectedSector( int i )
    						{ selsector_ = i; }

protected:

    Setup			setup_;
    Stats::DirectionalData	data_;

    bool			isempty_;
    Interval<float>		posrg_;
    Interval<float>		valrg_;
    int				maxcount_;
    uiPoint			center_;
    int				radius_;
    int				cursector_;
    int				selsector_;
    const ColTab::Sequence*	colseq_;

    uiGraphicsItemGroup&	sectorlines_;
    uiCircleItem*		outercircleitm_;
    uiCurvedItem*		selsectoritem_;
    uiTextItem*			hdrannotitm1_;
    uiTextItem*			hdrannotitm2_;
    ObjectSet<uiCircleItem>	equicircles_;
    ObjectSet<uiTextItem>	dirtxtitms_;
    ObjectSet<uiLineItem>	dirlnitms_;
    uiGraphicsItemGroup&	markeritems_;
    uiGraphicsItemGroup&	curveitems_;
    uiLineItem*			scalelineitm_;
    uiMarkerItem*		scalestartptitem_;
    uiCurvedItem*		scalearcitm_;
    uiTextItem*			scaleannotitm_;
    uiTextItem*			scalestartitm_;
    uiTextItem*			scalestopitm_;
    uiColTabItem*		coltabitm_;

    void			mouseRelease(CallBacker*);
    void			reSized(CallBacker*);

    void			gatherInfo();
    void			draw();
    void			drawGrid();
    void			drawData();
    void			drawScatter();
    void			drawVals();
    void			drawRose();
    void			drawSelection();
    uiCurvedItem*		drawSectorPart(int,Interval<float>,Color);
    void			drawSectorParts(bool);

    void			drawAnnot();
    void			drawDirAnnot();
    void			drawHeader();
    void			drawScale();
    void			drawColTab();

    uiPoint			dataUIPos(float r,float ang) const;
    uiPoint			usrUIPos(float r,float ang) const;

};


#endif


