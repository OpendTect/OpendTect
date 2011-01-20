/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.77 2011-01-20 10:21:39 cvsbruno Exp $";

#include "uiwelltieview.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiwelllogdisplay.h"
#include "uiwelldisplaycontrol.h"
#include "uiwelltiecontrolview.h"
#include "uiworld2ui.h"

#include "flatposdata.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbufadapters.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiepickset.h"


#define mGetWD(act) const Well::Data* wd = data_.wd_; if ( !wd ) act;
namespace WellTie
{

uiTieView::uiTieView( uiParent* p, uiFlatViewer* vwr, const Data& data )
	: vwr_(vwr)
	, parent_(p)
	, trcbuf_(*new SeisTrcBuf(true))	    
	, params_(data.dispparams_)
	, data_(data)				   
	, synthpickset_(data_.pickdata_.synthpicks_)
	, seispickset_(data_.pickdata_.seispicks_)
	, zrange_(data_.timeintv_)
	, checkshotitm_(0)
	, wellcontrol_(0)
	, infoMsgChanged(this)
{
    initFlatViewer();
    initLogViewers();
    initWellControl();
} 


uiTieView::~uiTieView()
{
    delete &trcbuf_;
}


void uiTieView::initWellControl()
{
    wellcontrol_ = new uiWellDisplayControl( *logsdisp_[0] );
    wellcontrol_->addLogDisplay( *logsdisp_[1] );
    wellcontrol_->posChanged.notify( mCB(this,uiTieView,setInfoMsg) );
}


void uiTieView::fullRedraw()
{
    setLogsParams();
    drawLog( data_.currvellog(), 0, 0, data_.isSonic() );
    drawLog( data_.density(), 1, 0, false );
    drawLog( data_.ai(), 0, 1, false );
    drawLog( data_.reflectivity(), 1, 1, true );
    drawLogDispWellMarkers();
    drawCShot();
    
    redrawViewer(0);
}


void uiTieView::redrawViewer( CallBacker* cb )
{
    mDynamicCastGet(uiControlView*,ctrl,vwr_->control())
    redrawViewerMarkers( cb );
    drawTraces();
    if ( ctrl ) 
	ctrl->setSelView( false, false );
}


void uiTieView::redrawViewerMarkers( CallBacker* )
{
    drawUserPicks();
    drawViewerWellMarkers();
    drawHorizons();
    vwr_->handleChange( FlatView::Viewer::Annot );    
}


void uiTieView::initLogViewers()
{
    for ( int idx=0; idx<2; idx++ )
    {
	uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
	wldsu.border_.setLeft(0); wldsu.border_.setRight(0);
	logsdisp_ += new uiWellLogDisplay( parent_, wldsu );
	logsdisp_[idx]->setPrefWidth( vwr_->prefHNrPics()/2 );
	logsdisp_[idx]->setPrefHeight( vwr_->prefVNrPics() );
	logsdisp_[idx]->disableScrollZoom();
    }
    logsdisp_[0]->attach( leftOf, logsdisp_[1] );
    logsdisp_[1]->attach( leftOf, vwr_ );
}


void uiTieView::initFlatViewer()
{
    vwr_->setInitialSize( uiSize(520,540) );
    vwr_->setExtraBorders( uiSize(0,0), uiSize(0,20) );
    FlatView::Appearance& app = vwr_->appearance();
    app.setDarkBG( false );
    app.setGeoDefaults( true );
    app.annot_.showaux_ = true ;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.sampling_ = 0.2;
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.clipperc_.set(0,0);
    app.ddpars_.wva_.overlap_ = 1;
    app.ddpars_.wva_.right_= app.ddpars_.wva_.wigg_ = Color::Black();
    app.annot_.x1_.name_ = data_.seismic();
    app.annot_.x2_.name_ = "TWT (ms)";
    app.annot_.title_ ="Synthetics<---------------------------------->Seismics";
    vwr_->viewChanged.notify( mCB(this,uiTieView,zoomChg) );
    vwr_->rgbCanvas().scene().getMouseEventHandler().movement.notify( 
	    				mCB( this, uiTieView, setInfoMsg ) );
}


void uiTieView::setLogsParams()
{
    mGetWD(return)
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->logData(true).wl_ = 0;
	logsdisp_[idx]->logData(false).wl_ = 0;
	uiWellDahDisplay::Data data;
	data.zrg_ = zrange_;
	data.d2tm_ = wd->d2TModel();
	data.markers_ = &wd->markers();
	data.dispzinft_ = params_.iszinft_;
	data.zistime_ = params_.iszintime_;
	logsdisp_[idx]->setData( data );
    }
    setLogsRanges( Interval<float>( zrange_.start, zrange_.stop ) );
}


void uiTieView::drawLog( const char* nm, int lognr, int dispnr, bool reversed )
{
    uiWellLogDisplay::LogData& wldld = logsdisp_[dispnr]->logData( lognr );
    wldld.wl_ = data_.logset_.getLog( nm );
    wldld.disp_.color_ = Color::stdDrawColor( lognr );
    wldld.disp_.isleftfill_ = wldld.disp_.isrightfill_ = false;
    wldld.xrev_ = reversed;
}


#define mNrTrcs 14
void uiTieView::drawTraces()
{
    trcbuf_.erase();
    const int nrtrcs = mNrTrcs;
    for ( int idx=0; idx<nrtrcs; idx++ )
    {
	const int midtrc = nrtrcs/2-1;
	const bool issynth = idx < midtrc;
	SeisTrc* trc = new SeisTrc( issynth ? data_.synthtrc_ : data_.seistrc_);
	trc->info().sampling.start = zrange_.start *1000;
	trc->info().sampling.step = zrange_.step *1000;
	trcbuf_.add( trc );
	bool udf = idx == 0 || idx == midtrc || idx == midtrc+1 || idx>nrtrcs-2;
	if ( udf ) 
	    setUdfTrc( *trc );
	else
	    { SeisTrcPropChg pc( *trc ); pc.normalize( true ); }
    }
    setDataPack();
}


void uiTieView::setUdfTrc( SeisTrc& trc ) const
{
    for ( int idx=0; idx<trc.size(); idx++)
	trc.set( idx, mUdf(float), 0 );
}


void uiTieView::setDataPack() 
{ 
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( &trcbuf_, Seis::Vol, 
				SeisTrcInfo::TrcNr, "Seismic" );
    dp->trcBufArr2D().setBufMine( false );
    StepInterval<double> xrange( 1, trcbuf_.size(), 1 );
    dp->posData().setRange( true, xrange );
    float fac = 1000;
    StepInterval<double> z(zrange_.start*fac,zrange_.stop*fac,zrange_.step*fac);
    dp->posData().setRange( false, z );
    dp->setName( data_.seismic() );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false, false );
}


void uiTieView::setLogsRanges( Interval<float> rg )
{
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( rg );
}


void uiTieView::zoomChg( CallBacker* )
{
    const uiWorldRect& curwr = vwr_->curView();
    setLogsRanges( Interval<float>( curwr.top(), curwr.bottom() ) );
    drawCShot();
}


void uiTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
		    bool left, float zpos, Color col, bool ispick, bool fullnm )
{
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.width_ = ispick ? 2 : 1;
    auxdata->linestyle_.type_  = ispick ? LineStyle::Solid : LineStyle::Dot;
    Interval<float> xrg(vwr_->boundingBox().left(),vwr_->boundingBox().right());
    auxdata->poly_ += FlatView::Point( left ? xrg.width()/2 : xrg.stop, zpos );
    auxdata->poly_ += FlatView::Point( left ? xrg.start : xrg.width()/2, zpos );

    if ( !ispick  )
    {
	BufferString mtxt( auxdata->name_ );
	if ( !fullnm && mtxt.size() > 3 )
	    mtxt[3] = '\0';
	auxdata->name_ = mtxt;
	auxdata->namealignment_ = Alignment(Alignment::HCenter,Alignment::Top);
	auxdata->namepos_ = 1;
    }
}	


void uiTieView::drawLogDispWellMarkers()
{
    mGetWD(return)
    const bool ismarkerdisp = params_.ismarkerdisp_;
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
	logsdisp_[idx]->setMarkers( ismarkerdisp ? &wd->markers() : 0 );
}


#define mRemoveItms( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
	vwr_->rgbCanvas().scene().removeItem( itms[idx] ); \
    deepErase( itms );
#define mRemoveSet( auxs ) \
    for ( int idx=0; idx<auxs.size(); idx++ ) \
        app.annot_.auxdata_ -= auxs[idx]; \
    deepErase( auxs );
void uiTieView::drawViewerWellMarkers()
{
    mRemoveItms( mrktxtnms_ )
    mGetWD(return)

    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( wellmarkerauxdatas_ );
    bool ismarkerdisp = params_.isvwrmarkerdisp_;
    if ( !ismarkerdisp ) return;
    const Well::D2TModel* d2tm = wd->d2TModel();
    if ( !d2tm ) return; 
    for ( int midx=0; midx<wd->markers().size(); midx++ )
    {
	const Well::Marker* marker = wd->markers()[midx];
	if ( !marker  ) continue;
	
	float zpos = d2tm->getTime( marker->dah() );
	const Color& col = marker->color();
	
	if ( !zrange_.includes( zpos ) )
	    continue;
	
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(marker->name()) );
	wellmarkerauxdatas_ += auxdata;
	app.annot_.auxdata_ +=  auxdata;
	zpos *= 1000;	
	drawMarker(auxdata, true, zpos, col, false, params_.disphorfullnames_);
    }
}	


void uiTieView::drawUserPicks()
{
    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( userpickauxdatas_ );
    const int nrauxs = mMAX( seispickset_.size(), synthpickset_.size() );
    
    for ( int idx=0; idx<nrauxs; idx++ )
    {
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
	userpickauxdatas_ += auxdata;
	app.annot_.auxdata_ +=  auxdata;
    }
    drawUserPicks( seispickset_, false );
    drawUserPicks( synthpickset_, true );

    vwr_->handleChange( FlatView::Viewer::Annot );    
}


void uiTieView::drawUserPicks( const TypeSet<Marker>& pickset, bool issynth )
{
    for ( int idx=0; idx<pickset.size(); idx++ )
    {
	const Marker& pick = pickset[idx];
	float zpos = pick.zpos_*1000; 
	drawMarker(userpickauxdatas_[idx],issynth,zpos,pick.color_,true,false);
    }
}


void uiTieView::drawHorizons()
{
    mRemoveItms( hortxtnms_ )
    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( horauxdatas_ );
    bool ishordisp = params_.isvwrhordisp_;
    if ( !ishordisp ) return;
    const TypeSet<Marker>& horizons = data_.horizons_;
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
	horauxdatas_ += auxdata;
	app.annot_.auxdata_ += auxdata;
	const Marker& hor = horizons[idx];
	float zval = hor.zpos_;
	auxdata->name_ = hor.name_;
	drawMarker( auxdata, false, zval, hor.color_, false, 
			params_.dispmrkfullnames_ );
    }
}


void uiTieView::drawCShot()
{
    uiGraphicsScene& scene = logsdisp_[0]->scene();
    scene.removeItem( checkshotitm_ );
    delete checkshotitm_; checkshotitm_=0;

    mGetWD(return)
    const Well::Log* cslog = wd->logs().getLog( data_.checkshotlog() );
    if ( !params_.iscsdisp_ || !cslog ) return;

    TypeSet<uiPoint> pts;
    uiWellLogDisplay::LogData& ld = logsdisp_[0]->logData(true);
    const Interval<float>& zrg = ld.zrg_;
    const Well::D2TModel* d2tm = wd->d2TModel();
    for ( int idx=0; idx<cslog->size(); idx++ )
    {
	const float val  = cslog->value( idx );
	float zpos = cslog->dah( idx );
	if ( !params_.iszintime_ ) 
	    zpos = d2tm ? d2tm->getDah( zpos ) : zpos;
	if ( zpos < zrg.start )
	    continue;
	else if ( zpos > zrg.stop )
	    break;
	if ( params_.iszinft_ ) zpos *= mToFeetFactor;
	pts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(zpos) );
    }
    if ( pts.isEmpty() ) return;
    checkshotitm_ = scene.addItem( new uiPolyLineItem(pts) );
    LineStyle ls( LineStyle::Solid, 2, Color::DgbColor() );
    checkshotitm_->setPenStyle( ls );
}


void uiTieView::setInfoMsg( CallBacker* cb )
{
    BufferString infomsg;
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh )
    { 
	mCBCapsuleUnpack(BufferString,inf,cb); 
	infomsg = inf;
    }
    CBCapsule<BufferString> caps( infomsg, this );
    infoMsgChanged.trigger( &caps );
}



uiCrossCorrView::uiCrossCorrView( uiParent* p, const Data& d )
	: uiGroup(p)
    	, data_(d)
{
    uiFunctionDisplay::Setup fdsu; 
    fdsu.border_.setLeft( 2 );		
    fdsu.border_.setRight( 0 );
    fdsu.epsaroundzero_ = 1e-3;
    disp_ = new uiFunctionDisplay( this, fdsu );
    disp_->xAxis()->setName( "Lags (ms)" );
    disp_->yAxis(false)->setName( "Coefficient" );
    lbl_ = new uiLabel( this,"" );
    lbl_->attach( centeredAbove, disp_ );
}


void uiCrossCorrView::set( float* arr, int sz, float lag, float coeff )
{
    vals_.erase();
    for ( int idx=0; idx<sz; idx++ )
	vals_ += arr[idx];
    lag_ = lag;
    coeff_ = coeff;
}


void uiCrossCorrView::draw()
{
    TypeSet<float> xvals;
    for ( int idx=0; idx<vals_.size(); idx++)
    {
	float xaxistime = idx*data_.timeintv_.step;
	if ( fabs( xaxistime ) > lag_ )
	    continue;
	xvals += xaxistime;
    }
    disp_->setVals( xvals.arr(), vals_.arr(), xvals.size() );
    BufferString corrbuf = "Cross-Correlation Coefficient: ";
    corrbuf += coeff_;
    lbl_->setPrefWidthInChar(50);
    lbl_->setText( corrbuf );
}

}; //namespace WellTie
