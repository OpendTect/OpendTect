/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uiwelllogdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "coltabseqmgr.h"
#include "coltabmapper.h"
#include "welllog.h"


//LogData
uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellLogDisplay::Setup& s )
    : uiWellDahDisplay::DahObjData( scn, isfirst, s )
    , logSet(this)
{
    disp_.setColor(Color::stdDrawColor( isfirst ? 0 : 1 ) );
}


void uiWellLogDisplay::gatherDataInfo( int logidx )
{
    LogData& ld = logData( logidx );
    ld.xax_.setup().islog( ld.disp_.isLogarithmic() );
    ld.cliprate_ = ld.disp_.clipRate();
    ld.valrg_ = ld.disp_.range();

    uiWellDahDisplay::gatherDataInfo( logidx );
}


const Well::Log* uiWellLogDisplay::LogData::log() const
{
    mDynamicCastGet(const Well::Log*,dahlog,dahobj_)
    return dahlog;
}


void uiWellLogDisplay::LogData::setLog( const Well::Log* l )
{
    if ( dahobj_ != l )
    {
	setData( l );
	logSet.trigger();
    }
}


void uiWellLogDisplay::LogData::getInfoForDah( float dah,
					       BufferString& msg ) const
{
    const Well::Log* wl = log();
    if ( wl )
    {
	msg.add( log()->name() ).add( ":" );
	msg.add( toString(log()->valueAt(dah)) );
	msg.add( log()->unitMeasLabel() );
    }
}


//uiWellLogDisplay
uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiWellDahDisplay(p,su)
    , setup_(su)
{
    deepErase( logsdata_ );
    logsdata_ += new LogData( scene(), true, su );
    logsdata_ += new LogData( scene(), false, su );

    postFinalise().notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su,
			   const Well::DisplayProperties2D::LogPanelProps& lp )
    :uiWellDahDisplay(p,su)
    , setup_(su)
{
    deepErase( logsdata_ );
    //TODO logsdata_ size = lp.logs_.size()
    logsdata_ += new LogData( scene(), true, su );
    logsdata_ += new LogData( scene(), true, su );

    mAttachCB(lp.objectChanged(),uiWellLogDisplay::setLogDisplayPorp);

    postFinalise().notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
    deepErase( logsdata_ );
}


void uiWellLogDisplay::drawData( int ldidx )
{
    uiWellDahDisplay::draw();//TODO need to change in base class function

    int style = logData(ldidx).disp_.style();
    if ( style==0 || style==2 )
	drawFilledCurve( ldidx );
    else
	drawSeismicCurve( ldidx );

    style = logData(false).disp_.style();
    if ( style==0 || style==2 )
	drawFilledCurve( ldidx );
    else
	drawSeismicCurve( ldidx );
}


void uiWellLogDisplay::setLogDisplayPorp( CallBacker* cb )
{
    mGetMonitoredChgData(cb,chgdata);
    if ( !cb )
	return;

    //const bool changedprop = chgdata.includes( Well::DisplayProperties2D
//					       ::cLogPanelAdded() );
}


void uiWellLogDisplay::drawCurve( int ldidx )
{
    //TODO need to change in base class function
    uiWellDahDisplay::drawCurve( ldidx );
    LogData& ld = logData( ldidx );

    if ( !ld.curvepolyitm_ ) return;
    OD::LineStyle ls(OD::LineStyle::Solid);
    ls.width_ = ld.disp_.size();
    ls.color_ = ld.disp_.color();
    ld.curvepolyitm_->setPenStyle( ls );
    ld.curvepolyitm_->setVisible( ls.width_ > 0 );
}


static const int cMaxNrLogSamples = 2000;
#define mGetWLSizeAndStep() \
    if ( !ld.log() ) \
	return; \
    const Well::Log& wl = *ld.log(); \
    MonitorLock ml( wl ); \
    int sz = wl.size(); \
    if ( sz < 2 ) \
	return; \
    float step = 1.f; \
    if ( sz > cMaxNrLogSamples ) \
    { \
	step = ((float)sz) / cMaxNrLogSamples; \
	sz = cMaxNrLogSamples; \
    }\



void uiWellLogDisplay::drawSeismicCurve( int ldidx )
{
    uiWellLogDisplay::LogData& ld = logData( ldidx );
    deepErase( ld.curvepolyitms_ );

    const float rgstop = ld.xax_.range().stop;
    const float rgstart = ld.xax_.range().start;

    mGetWLSizeAndStep();

    ObjectSet< TypeSet<uiPoint> > pts;
    uiPoint closept;

    float zfirst = wl.firstDah();
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
	const int index = mNINT32(idx*step);
	float dah = wl.dahByIdx( index );
	if ( index && index < sz-1 )
	{
	    if ( dah>=wl.dahByIdx(index+1) || dah<=wl.dahByIdx(index-1) )
		continue;
	}
	mDefZPosInLoop( dah )

	float val = wl.valueByIdx( index );

	pt.x_ = ld.xax_.getPix(val);
	pt.y_ = closept.y_ = ld.yax_.getPix(zpos);

	if ( mIsUdf(val) || pt.x_ < closept.x_ )
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
    *pts[pts.size()-1] += uiPoint( closept.x_, pt.y_ );

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	const Color color = ld.disp_.seisColor();
	pli->setFillColor( color );
	pli->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,1,color) );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


void uiWellLogDisplay::drawFilledCurve( int ldidx )
{
    uiWellLogDisplay::LogData& ld = logData( ldidx );
    deepErase( ld.curvepolyitms_ );

    const bool fillleft = ld.disp_.fillLeft();
    const bool fillright = ld.disp_.fillRight();
    if ( !fillleft && !fillright )
	return;

    const float rgstop = ld.xax_.range().stop;
    const float rgstart = ld.xax_.range().start;
    const bool isrev = rgstop < rgstart;

    ConstRefMan<ColTab::Sequence> seq = ColTab::SeqMGR().getAny(
							ld.disp_.seqName() );
    const Interval<float> fillrg = ld.disp_.fillRange();
    const ColTab::SeqUseMode sequsemode = ld.disp_.seqUseMode();
    const int nrsegs = seq->nrSegments();
    mGetWLSizeAndStep();

    const bool fullpanelfill = fillleft && fillright;
    const bool first = ldidx==0;
    const bool isfillrev = !fullpanelfill &&
		 ( ( first && fillleft && !isrev )
		|| ( first && fillright && isrev )
		|| ( !first && fillright && !isrev )
		|| ( !first && fillleft && isrev ) );

    float zfirst = wl.firstDah();
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    uiPoint closept;
    closept.x_ = ( first ) ? isfillrev ? pixstart : pixstop
			   : isfillrev ? pixstop  : pixstart;
    closept.y_ = ld.yax_.getPix( zfirst );
    float prevcolpos = mUdf(float);
    TypeSet<float> colorposset;
    ObjectSet<TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    uiPoint pt;
    RefMan<ColTab::Mapper> mapper = new ColTab::Mapper( fillrg );
    mapper->setup().setSeqUseMode( sequsemode );
    mapper->setup().setNrSegs( nrsegs );

    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT32(idx*step);
	float dah = wl.dahByIdx( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= wl.dahByIdx(index+1) || dah <= wl.dahByIdx(index-1) )
		continue;
	}
	mDefZPosInLoop( dah )

	const float val = wl.valueByIdx( index );
	const bool isvaludf = mIsUdf(val);
	float colpos = mUdf( float );
	if ( !isvaludf )
	    colpos = mapper->seqPosition( val );

	if ( mIsUdf(prevcolpos) )
	    prevcolpos = colpos;
	float valforx = val;
	if ( fullpanelfill )
	    valforx = first ? rgstart : rgstop;

	if ( !mIsUdf(valforx) )
	{
	    pt.x_ = ld.xax_.getPix(valforx);
	    pt.y_ = ld.yax_.getPix(zpos);
	    if ( curpts->isEmpty() )
		*curpts += uiPoint( closept.x_, pt.y_ );
	    *curpts += pt;
	}

	if ( !mIsEqual(colpos,prevcolpos,mDefEps) )
	{
	    *curpts += uiPoint( closept.x_, pt.y_ );
	    pts += curpts;
	    colorposset += prevcolpos;
	    prevcolpos = colpos;

	    curpts = new TypeSet<uiPoint>;
	    if ( mIsUdf(colpos) ) continue;
	    *curpts += uiPoint( closept.x_, pt.y_ );
	    *curpts += pt;
	}
    }

    if ( !pts.isEmpty() )
    {
	*curpts += uiPoint( closept.x_, pt.y_ );
	pts += curpts;
	colorposset += prevcolpos;
    }

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color = ld.disp_.singleColor() ? ld.disp_.seisColor()
					    : seq->color(colorposset[idx]);
	pli->setFillColor( color );
	pli->setPenStyle( OD::LineStyle(OD::LineStyle::None) );
	pli->setZValue( 1 );
    }

    deepErase( pts );
}


uiWellLogDisplay::LogData& uiWellLogDisplay::logData( int ldidx )
{
    return *static_cast<LogData*>( logsdata_.get(ldidx) );
}


int uiWellLogDisplay::nrLogs() const
{
    return logsdata_.size();
}


//uiWellLogDispDlg
uiWellLogDispDlg::uiWellLogDispDlg( uiParent* p,
				    const uiWellLogDisplay::Setup& wldsu )
    : uiDialog(p,uiDialog::Setup(uiString::empty(),mNoDlgTitle,mNoHelpKey)
		    .modal(false))
    , logSet(this)
{
    setCtrlStyle( CloseOnly );
    dispfld_ = new uiWellLogDisplay( this, wldsu );
    const CallBack cb( mCB(this,uiWellLogDispDlg,logSetCB) );
    dispfld_->logData(true).logSet.notify( cb );
    dispfld_->logData(false).logSet.notify( cb );
    dispfld_->dahObjData(true).col_ = dispfld_->logData(true).disp_.color();
    dispfld_->dahObjData(false).col_ = dispfld_->logData(false).disp_.color();

    dispfld_->setPrefWidth( 300 );
    dispfld_->setPrefHeight( 500 );
    postFinalise().notify( cb );
    setDeleteOnClose( true );
}


uiWellLogDispDlg::~uiWellLogDispDlg()
{
}


void uiWellLogDispDlg::setLog( const Well::Log* wl, bool first,
			       const char* wellnm )
{
    (first ? wellnm1_ : wellnm2_).set( wellnm );
    dispfld_->logData( first ).setLog( wl );
}


const Well::Log* uiWellLogDispDlg::getLog( bool first ) const
{
    return dispfld_->logData( first ).log();
}


void uiWellLogDispDlg::logSetCB( CallBacker* )
{
    const Well::Log* l1 = getLog( true );
    const Well::Log* l2 = getLog( false );

    const bool have2wells = !wellnm1_.isEmpty() && !wellnm2_.isEmpty()
			    && wellnm1_ != wellnm2_;
    const bool have2logs = l1 && l2 && l1 != l2;
    const bool samelognm = have2logs && l1->hasName( l2->name() );

    uiString capt;
    if ( have2wells && have2logs && !samelognm )
    {
	capt = toUiString("%1 [%2] | %3 [%4]")
		.arg( wellnm1_ ).arg( l1->name() )
		.arg( wellnm2_ ).arg( l2->name() );
    }
    else
    {
	uiString logstr;
	if ( !have2logs || samelognm )
	    logstr = toUiString( (l1 ? l1 : l2)->name() );
	else
	    logstr = toUiString("%1|%2").arg(l1->name()).arg(l2->name());
	uiString wellstr;
	if ( !have2wells )
	    wellstr = toUiString(wellnm1_.isEmpty() ? wellnm2_ : wellnm1_);
	else
	    wellstr = toUiString( "%1|%2" ).arg( wellnm1_ ).arg( wellnm2_ );

	capt = toUiString("%1 [%2]").arg( wellstr ).arg( logstr );
    }

    setCaption( capt );

    if ( l1 && !wellnm1_.isEmpty() )
    {
	capt = toUiString("%1 [%2]").arg(wellnm1_).arg(l1->name());
	dispfld_->dahObjData( true ).xax_.setCaption( capt );
    }
    if ( l2 && !wellnm2_.isEmpty() )
    {
	capt = toUiString("%1 [%2]").arg(wellnm2_).arg(l2->name());
	dispfld_->dahObjData( false ).xax_.setCaption( capt );
    }

    logSet.trigger();
}


uiWellLogDispDlg* uiWellLogDispDlg::popupNonModal( uiParent* p,
				const Well::Log* wl1, const Well::Log* wl2,
				const char* wellnm1,
				const char* wellnm2)
{
    uiWellLogDisplay::Setup wldsu;
    wldsu.annotinside( false ).drawcurvenames( false );
    uiWellLogDispDlg* dlg = new uiWellLogDispDlg( p, wldsu );
    dlg->setLog( wl1, true, wellnm1 );
    if ( wl2 )
	dlg->setLog( wl2, false, wellnm2 );

    dlg->setDeleteOnClose( true );
    dlg->show();
    return dlg;
}
