#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

mExpClass(uiTools) uiDirectionalPlot : public uiGraphicsView
{ mODTextTranslationClass(uiDirectionalPlot)
public:

    mStruct(uiTools) Setup
    {
	enum Type		{ Rose, Scatter, Vals };
			Setup( Type t=Rose )
			    : type_(t)
			    , circlels_(OD::LineStyle::Solid)
			    , sectorls_(OD::LineStyle::Solid)
			    , equils_(OD::LineStyle::Dot)
			    , markstyle_(MarkerStyle2D::Circle)
			    , hlmarkstyle_(MarkerStyle2D::Square,4,
					   OD::Color::Green())
			    , docount_(false)
			    , curissel_(true)
			    , valcolor_(true)
			    , clipratio_(0.02)
			    , prefsize_(400,400)
			{}
			~Setup()
			{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(OD::LineStyle,circlels)
	mDefSetupMemb(OD::LineStyle,sectorls)
	mDefSetupMemb(OD::LineStyle,equils)
	mDefSetupMemb(MarkerStyle2D,markstyle)
	mDefSetupMemb(MarkerStyle2D,hlmarkstyle) // Highlight marker style
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

    void			setColTab(const char* nm);
    void			showColTabItem(bool);
    void			showScaleItem(bool);

    void			setNrEquicircles(int);
    void			getMousePosInfo(int& count,float& angle,
						float& pos);

    void			setHighlighted(const TypeSet<int>&);
    void			setHighlighted(int);

protected:

    Setup			setup_;
    Stats::DirectionalData	data_;
    TypeSet<int>		highlightidxs_;

    bool			isempty_			= true;
    Interval<float>		posrg_;
    Interval<float>		valrg_;
    int				maxcount_			= 0;
    int				nrequicircles_			= 3;
    uiPoint			center_;
    int				radius_;
    int				cursector_			= -1;
    int				selsector_			= -1;
    const ColTab::Sequence*	colseq_				= nullptr;

    uiGraphicsItemGroup&	sectorlines_;
    uiCircleItem*		outercircleitm_			= nullptr;
    uiCurvedItem*		selsectoritem_			= nullptr;
    uiTextItem*			hdrannotitm1_			= nullptr;
    uiTextItem*			hdrannotitm2_			= nullptr;
    ObjectSet<uiCircleItem>	equicircles_;
    ObjectSet<uiTextItem>	dirtxtitms_;
    ObjectSet<uiLineItem>	dirlnitms_;
    uiGraphicsItemGroup&	markeritems_;
    uiGraphicsItemGroup&	curveitems_;
    uiLineItem*			scalelineitm_			= nullptr;
    uiMarkerItem*		scalestartptitem_		= nullptr;
    uiCurvedItem*		scalearcitm_			= nullptr;
    uiTextItem*			scaleannotitm_			= nullptr;
    uiTextItem*			scalestartitm_			= nullptr;
    uiTextItem*			scalestopitm_			= nullptr;
    uiColTabItem*		coltabitm_			= nullptr;

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
    uiCurvedItem*		drawSectorPart(int,Interval<float>,OD::Color);
    void			drawSectorParts(bool);

    void			drawAnnot();
    void			drawDirAnnot();
    void			drawHeader();
    void			drawScale();
    void			drawColTab();

    uiPoint			dataUIPos(float r,float ang) const;
    uiPoint			usrUIPos(float r,float ang) const;

};
