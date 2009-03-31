#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uidirectionalplot.h,v 1.1 2009-03-31 12:13:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "statdirdata.h"
#include "draw.h"
class uiGraphicsItem;
class uiGraphicsItemGroup;

/*!\brief creates a directional plot: Rose diagrams and more.

  The data will be plotted as a Rose diagram if there is exactly one value per
  sector.

 */

mClass uiDirectionalPlot : public uiGraphicsView
{
public:

    struct Setup
    {
				Setup()
				    : circlels_(LineStyle::Solid)
				    , sectorls_(LineStyle::Solid)
				    , equils_(LineStyle::None)
				    , drawdirannot_(true)
				    , drawposannot_(false)
				    , prefsize_(400,400)	{}

	mDefSetupMemb(LineStyle,circlels)
	mDefSetupMemb(LineStyle,sectorls)
	mDefSetupMemb(LineStyle,equils)
	mDefSetupMemb(bool,drawdirannot)
	mDefSetupMemb(bool,drawposannot)
	mDefSetupMemb(uiSize,prefsize)
    };

				uiDirectionalPlot(uiParent*,const Setup&);
				~uiDirectionalPlot();

    void			setVals(const Stats::DirectionalData&);
    void			setVals(const float*,int);	//!< Rose dgrm
    Stats::DirectionalData::Setup& dataSetup()	{ return data_.setup_; }

    Setup&			setup()		{ return setup_; }
    const Stats::DirectionalData& data() const	{ return data_; }

    int				nrSectors() const { return data().size(); }
    int				size() const	{ return nrSectors(); }
    float			angle( int s ) const { return data_.angle(s); }
    bool			isRose() const;

    Notifier<uiDirectionalPlot>	sectorPartSelected;
    int				selSector() const { return selsector_; }
    int				selPart() const	{ return selpart_; }

protected:

    Setup			setup_;
    Stats::DirectionalData	data_;
    mutable int			selsector_;
    mutable int			selpart_;

    uiGraphicsItem*		ypolyitem_;
    uiGraphicsItemGroup*	equicircles_;
    uiGraphicsItemGroup*	sectorlines_;

    void			mouseRelease(CallBacker*);
    void			reSized(CallBacker*);

    void			gatherInfo();
    void			draw();

};


#endif
