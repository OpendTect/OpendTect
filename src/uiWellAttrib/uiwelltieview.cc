/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltieview.h"
#include "uiwelltiecontrolview.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiwelllogdisplay.h"
#include "uiwelldisplaycontrol.h"
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
	, seisdp_(0)			 
	, infoMsgChanged(this)
{
    initFlatViewer();
    initLogViewers();
    initWellControl();
} 


uiTieView::~uiTieView()
{
    delete wellcontrol_;
    vwr_->clearAllPacks();
    delete &trcbuf_;
}


void uiTieView::initWellControl()
{
    wellcontrol_ = new uiWellDisplayControl( *logsdisp_[0] );
    wellcontrol_->addDahDisplay( *logsdisp_[1] );
    wellcontrol_->posChanged.notify( mCB(this,uiTieView,setInfoMsg) );
}


void uiTieView::fullRedraw()
{
    setLogsParams();
    drawLog( data_.sonic(), true, 0, !data_.isSonic() );
    drawLog( data_.density(), false, 0, false );
    drawLog( data_.ai(), true, 1, false );
    drawLog( data_.reflectivity(), false, 1, false );
    drawLogDispWellMarkers();
    
    redrawViewer();
}


void uiTieView::redrawViewer()
{
    drawTraces();
    redrawViewerAuxDatas();

    vwr_->handleChange( FlatView::Viewer::All );
    mDynamicCastGet(uiControlView*,ctrl,vwr_->control())
    if ( ctrl )
	ctrl->setSelView( false, false );
    zoomChg(0);
}


void uiTieView::redrawViewerAuxDatas()
{
    drawUserPicks();
    drawViewerWellMarkers();
    drawHorizons();
    vwr_->handleChange( FlatView::Viewer::Auxdata );
}


void uiTieView::redrawLogsAuxDatas()
{
    drawLogDispWellMarkers();
}


void uiTieView::initLogViewers()
{
    for ( int idx=0; idx<2; idx++ )
    {
	uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
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
    app.annot_.x2_.showgridlines_ = true;
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.mappersetup_.cliprate_.set(0.0,0.0);
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
	logsdisp_[idx]->logData(true).setLog( 0 );
	logsdisp_[idx]->logData(false).setLog( 0 );
	uiWellDahDisplay::Data data;
	data.wd_ = wd;
	data.dispzinft_ = params_.iszinft_;
	data.zistime_ = params_.iszintime_;
	logsdisp_[idx]->setData( data );
    }
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    Interval<float> zrg( zrange_.start*zfac, zrange_.stop*zfac );
    setLogsRanges( zrg );
}


void uiTieView::drawLog( const char* nm, bool first, int dispnr, bool reversed )
{
    uiWellLogDisplay::LogData& wldld = logsdisp_[dispnr]->logData( first );
    wldld.setLog( data_.logset_.getLog( nm ) );
    wldld.disp_.color_ = Color::stdDrawColor( first ? 0 : 1 );
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
	SeisTrc* trc = new SeisTrc;
	trc->copyDataFrom( issynth ? data_.synthtrc_ : data_.seistrc_ );
	trc->info().sampling = data_.seistrc_.info().sampling;
	trc->info().sampling.scale( mCast(float,SI().zDomain().userFactor()) );
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
    vwr_->clearAllPacks();
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( &trcbuf_, Seis::Vol, 
				SeisTrcInfo::TrcNr, "Seismic" );
    dp->trcBufArr2D().setBufMine( false );
    StepInterval<double> xrange( 1, trcbuf_.size(), 1 );
    dp->posData().setRange( true, xrange );
    dp->setName( data_.seismic() );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false, false );
    vwr_->setPack( false, dp->id(), false, false );
}


void uiTieView::setLogsRanges( Interval<float> rg )
{
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( rg );
}


void uiTieView::zoomChg( CallBacker* )
{
    const uiWorldRect& curwr = vwr_->curView();
    Interval<float> zrg( (float) curwr.top(), (float) curwr.bottom() );
    setLogsRanges( zrg );
}


void uiTieView::drawMarker( FlatView::AuxData* auxdata,
				bool left, float zpos )
{
    Interval<float> xrg( (float) vwr_->boundingBox().left(),
				    (float) vwr_->boundingBox().right() );
    auxdata->poly_ += FlatView::Point( left ? xrg.width()/2 : xrg.stop, zpos );
    auxdata->poly_ += FlatView::Point( left ? xrg.start : xrg.width()/2, zpos );
}	


void uiTieView::drawLogDispWellMarkers()
{
    mGetWD(return)
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->markerDisp() = data_.dispparams_.mrkdisp_; 
	logsdisp_[idx]->reDrawAnnots();
    }
}

#define mRemoveItms( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
	vwr_->rgbCanvas().scene().removeItem( itms[idx] ); \
    deepErase( itms );
#define mRemoveSet( auxs ) \
    for ( int idx=0; idx<auxs.size(); idx++ ) \
        vwr_->removeAuxData( auxs[idx] ); \
    deepErase( auxs );
void uiTieView::drawViewerWellMarkers()
{
    mRemoveItms( mrktxtnms_ )
    mGetWD(return)

    mRemoveSet( wellmarkerauxdatas_ );
    bool ismarkerdisp = params_.isvwrmarkerdisp_;
    if ( !ismarkerdisp ) return;

    const Well::D2TModel* d2tm = wd->d2TModel();
    if ( !d2tm ) return; 
    for ( int midx=0; midx<wd->markers().size(); midx++ )
    {
	const Well::Marker* marker = wd->markers()[midx];
	if ( !marker  ) continue;

	const Well::DisplayProperties::Markers& mrkdisp 
	    					= data_.dispparams_.mrkdisp_;
	if ( !mrkdisp.selmarkernms_.isPresent( marker->name() ) )
	    continue;
	
	float zpos = d2tm->getTime( marker->dah(), wd->track() );
	
	if ( !zrange_.includes( zpos, true ) )
	    continue;

	const Color& col = mrkdisp.issinglecol_ ? mrkdisp.color_ 
	    					: marker->color();

	if ( col == Color::NoColor() || col == Color::White() )
	    continue;

	FlatView::AuxData* auxdata = vwr_->createAuxData( marker->name() );
	if ( !auxdata ) continue;

	wellmarkerauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
	zpos *= SI().zDomain().userFactor();
	const int shapeint = mrkdisp.shapeint_;
	const int drawsize = mrkdisp.size_;
	LineStyle ls = LineStyle( LineStyle::Dot, drawsize, col );
	if ( shapeint == 1 )
	    ls.type_ =  LineStyle::Solid;
	if ( shapeint == 2 )
	    ls.type_ = LineStyle::Dash;
	auxdata->linestyle_ = ls;

	BufferString mtxt( marker->name() );
	if ( !params_.dispmrkfullnames_ && mtxt.size() > 3 )
	    mtxt[3] = '\0';
	auxdata->name_ = mtxt;
	auxdata->namealignment_ = Alignment(Alignment::HCenter,Alignment::Top);
	auxdata->namepos_ = 1;
    
	drawMarker( auxdata, true, zpos );
    }
}	


void uiTieView::drawUserPicks()
{
    mRemoveSet( userpickauxdatas_ );
    const int nrauxs = mMAX( seispickset_.size(), synthpickset_.size() );
    
    for ( int idx=0; idx<nrauxs; idx++ )
    {
	FlatView::AuxData* auxdata = vwr_->createAuxData( 0 );
	userpickauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
    }
    drawUserPicks( seispickset_, false );
    drawUserPicks( synthpickset_, true );
}


void uiTieView::drawUserPicks( const TypeSet<Marker>& pickset, bool issynth )
{
    for ( int idx=0; idx<pickset.size(); idx++ )
    {
	const Marker& pick = pickset[idx];
	float zpos = pick.zpos_* SI().zDomain().userFactor();
	LineStyle ls = LineStyle( LineStyle::Solid, pick.size_, pick.color_ );
	userpickauxdatas_[idx]->linestyle_ = ls;
	drawMarker(userpickauxdatas_[idx], issynth, zpos );
    }
}


void uiTieView::drawHorizons()
{
    mRemoveItms( hortxtnms_ )
    mRemoveSet( horauxdatas_ );
    bool ishordisp = params_.isvwrhordisp_;
    if ( !ishordisp ) return;
    const TypeSet<Marker>& horizons = data_.horizons_;
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	FlatView::AuxData* auxdata = vwr_->createAuxData( 0 );
	horauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
	const Marker& hor = horizons[idx];
	float zval = hor.zpos_;

	BufferString mtxt( hor.name_ );
	if ( !params_.disphorfullnames_ && mtxt.size() > 3 )
	    mtxt[3] = '\0';
	auxdata->name_ = mtxt;
	auxdata->namealignment_ = Alignment(Alignment::HCenter,Alignment::Top);
	auxdata->namepos_ = 1;
	LineStyle ls = LineStyle( LineStyle::Dot, 2, hor.color_ );
	auxdata->linestyle_ = ls;
    
	drawMarker( auxdata, false, zval );
    }
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


void uiTieView::enableCtrlNotifiers( bool yn )
{
    logsdisp_[0]->scene().getMouseEventHandler().movement.enable( yn );
    logsdisp_[1]->scene().getMouseEventHandler().movement.enable( yn );
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


void uiCrossCorrView::set( const Data::CorrelData& cd )
{
    vals_.erase();
    for ( int idx=0; idx<cd.vals_.size(); idx++ )
	vals_ += cd.vals_[idx];
    lag_ = mCast( float, cd.lag_ );
    coeff_ = (float) cd.coeff_;
}


void uiCrossCorrView::draw()
{
    const int halfsz = vals_.size()/2;
    if ( !vals_.validIdx( halfsz ) )
	return;

    const float normalfactor = coeff_ / vals_[halfsz];
    TypeSet<float> xvals, yvals;
    for ( int idx=-halfsz; idx<halfsz; idx++)
    {
	float xaxistime = idx*data_.timeintv_.step*SI().zDomain().userFactor();
	if ( fabs( xaxistime ) > lag_ )
	    continue;
	xvals += xaxistime;
	float val = vals_[idx+halfsz];
	val *= normalfactor;
	yvals += fabs(val)>1 ? 0 : val;
    }
    disp_->setVals( xvals.arr(), yvals.arr(), xvals.size() );
    BufferString corrbuf = "Cross-Correlation Coefficient: ";
    corrbuf += coeff_;
    lbl_->setPrefWidthInChar(50);
    lbl_->setText( corrbuf );
}

}; //namespace WellTie
