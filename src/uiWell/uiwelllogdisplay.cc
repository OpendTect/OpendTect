/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiwelllogdisplay.cc,v 1.94 2012-06-22 09:09:35 cvsbruno Exp $";



#include "uiwelllogdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "coltabsequence.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#include <iostream>


uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellLogDisplay::Setup& s )
    : uiWellDahDisplay::DahObjData( scn, isfirst, s )  
    , unitmeas_(0)
{}


uiWellLogDisplay::~uiWellLogDisplay()
{
    delete ld1_; ld1_ = 0;
    delete ld2_; ld2_ = 0;
}

void uiWellLogDisplay::gatherDataInfo( bool first )
{
    LogData& ld = logData( first );
    ld.yax_.setup().islog( ld.disp_.islogarithmic_ );
    ld.cliprate_ = ld.disp_.cliprate_; 
    ld.valrg_ = ld.disp_.range_;

    uiWellDahDisplay::gatherDataInfo( first );
}


const Well::Log* uiWellLogDisplay::LogData::log() const
{
    mDynamicCastGet(const Well::Log*,dahlog,dahobj_)
    return dahlog;
}


void uiWellLogDisplay::LogData::setLog( const Well::Log* l )
{
    dahobj_ = l;
}


void uiWellLogDisplay::LogData::getInfoForDah( float dah, 
						BufferString& msg ) const
{
    if ( !log() ) return;
    msg += log()->name();
    msg += ":";
    msg += toString( log()->getValue( dah ) );
    msg += log()->unitMeasLabel();
}



uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiWellDahDisplay(p,su)
    , setup_(su)
{
    delete ld1_; delete ld2_;
    ld1_ = new LogData( scene(), true, su );
    ld2_ = new LogData( scene(), false, su );
    postFinalise().notify( mCB(this,uiWellLogDisplay,init) );
}


void uiWellLogDisplay::draw()
{
    uiWellDahDisplay::draw();

    if ( logData().disp_.iswelllog_ )
	drawFilledCurve( true );
    else
	drawSeismicCurve( true );

    if ( logData(false).disp_.iswelllog_ )
	drawFilledCurve( false );
    else
	drawSeismicCurve( false );
}


static const int cMaxNrLogSamples = 2000;
#define mGetLoopSize(nrsamp,step)\
    {\
	if ( nrsamp > cMaxNrLogSamples )\
	{\
	    step = (float)nrsamp/cMaxNrLogSamples;\
	    nrsamp = cMaxNrLogSamples;\
	}\
    }

void uiWellLogDisplay::drawCurve( bool first )
{
    uiWellDahDisplay::drawCurve( first );
    LogData& ld = logData( first );

    if ( !ld.curvepolyitm_ ) return;
    LineStyle ls(LineStyle::Solid);
    ls.width_ = ld.disp_.size_;
    ls.color_ = ld.disp_.color_;
    ld.curvepolyitm_->setPenStyle( ls );
    ld.curvepolyitm_->setVisible( ls.width_ > 0 );
}


void uiWellLogDisplay::drawSeismicCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = logData(first);
    deepErase( ld.curvepolyitms_ );

    if ( ld.disp_.iswelllog_ ) return;

    const float rgstop = ld.xax_.range().stop; 
    const float rgstart = ld.xax_.range().start;
    const bool isrev = rgstop < rgstart;

    int sz = ld.log() ? ld.log()->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    uiPoint closept;

    float zfirst = ld.log()->dah(0);
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    const int midpt = (int)( (pixstop-pixstart)/2 );
    closept = uiPoint( midpt, ld.yax_.getPix( zfirst ) );

    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept;
    uiPoint pt;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT(idx*step);
	float dah = ld.log()->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= ld.log()->dah(index+1) || dah <= ld.log()->dah(index-1) )
		continue;
	}
	mDefZPosInLoop( dah )

	float val = ld.log()->value( index );

	pt.x = ld.xax_.getPix(val);
	pt.y = closept.y = ld.yax_.getPix(zpos);

	if ( mIsUdf(val) || pt.x < closept.x )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
		*curpts += closept;
	    }
	    continue;
	}
	*curpts += closept;
	*curpts += pt;
	*curpts += closept;
    }
    if ( pts.isEmpty() ) return;
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y );

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color = ld.disp_.seiscolor_;
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


void uiWellLogDisplay::drawFilledCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = logData(first);
    deepErase( ld.curvepolyitms_ );

    if ( !ld.disp_.isleftfill_ && !ld.disp_.isrightfill_ ) return;

    const float rgstop = ld.xax_.range().stop; 
    const float rgstart = ld.xax_.range().start;
    const bool isrev = rgstop < rgstart;

    const float colrgstop = ld.disp_.fillrange_.stop; 
    const float colrgstart = ld.disp_.fillrange_.start;
    const bool iscolrev = colrgstop < colrgstart; 

    const float colstep = ( colrgstop - colrgstart ) / 255;
    int sz = ld.log() ? ld.log()->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;

    const bool fullpanelfill = ld.disp_.isleftfill_ && ld.disp_.isrightfill_;
    const bool isfillrev = !fullpanelfill &&  
		 ( ( first && ld.disp_.isleftfill_ && !isrev )
		|| ( first && ld.disp_.isrightfill_ && isrev )
		|| ( !first && ld.disp_.isrightfill_ && !isrev )
		|| ( !first && ld.disp_.isleftfill_ && isrev ) );

    float zfirst = ld.log()->dah(0);
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    closept.x = ( first ) ? isfillrev ? pixstart : pixstop 
			  : isfillrev ? pixstop  : pixstart;
    closept.y = ld.yax_.getPix( zfirst );
    int prevcolidx = 0;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept;

    uiPoint pt; uiPoint prevpt = closept;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT(idx*step);
	float dah = ld.log()->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= ld.log()->dah(index+1) || dah <= ld.log()->dah(index-1))
		continue;
	}
	mDefZPosInLoop( dah )

	float val = ld.log()->value( index );
	bool isvalrev = iscolrev;
	if ( ld.disp_.iscoltabflipped_ )
	    isvalrev = !isvalrev;
	const float valdiff = isvalrev ? colrgstop-val : val-colrgstart;
	int colindex = (int)( valdiff/colstep );
	if ( colindex > 255 ) colindex = 255; 
	if ( colindex < 0 )   colindex = 0; 
	if ( fullpanelfill ) 
	    val = first ? rgstart : rgstop;

	if ( mIsUdf(val) )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
		colorintset += 0;
	    }
	    continue;
	}

	pt.x = ld.xax_.getPix(val);
	pt.y = ld.yax_.getPix(zpos);
	
	*curpts += prevpt;
	*curpts += pt;

	prevpt = pt;

	if ( colindex != prevcolidx )
	{
	    *curpts += uiPoint( closept.x, pt.y );
	    colorintset += colindex;
	    prevcolidx = colindex;
	    pts += curpts;
	    curpts = new TypeSet<uiPoint>;
	    *curpts += uiPoint( closept.x, pt.y );
	}
    }
    if ( pts.isEmpty() ) return;
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y );

    const int tabidx = ColTab::SM().indexOf( ld.disp_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color =
	ld.disp_.issinglecol_ ? ld.disp_.seiscolor_
			      : seq->color( (float)colorintset[idx]/255 );
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


const uiWellLogDisplay::LogData& uiWellLogDisplay::logData( bool first ) const
{ return const_cast<uiWellLogDisplay*>(this)->logData(first); }


uiWellLogDisplay::LogData& uiWellLogDisplay::logData( bool first )
{ return *static_cast<LogData*>( first ? ld1_ : ld2_ ); }
