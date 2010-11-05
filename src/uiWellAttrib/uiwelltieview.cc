/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.74 2010-11-05 12:46:28 cvsbruno Exp $";

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
#include "welltiesetup.h"
#include "welltieunitfactors.h"


#define mGetWD(act) const Well::Data* wd = dataholder_.wd(); if ( !wd ) act;
namespace WellTie
{

uiTieView::uiTieView( uiParent* p, uiFlatViewer* vwr, 
		      WellTie::DataHolder& dh,
		      ObjectSet<uiWellLogDisplay>* ldis )  
	: vwr_(vwr)  
	, logsdisp_(*ldis)	     
	, dataholder_(dh)  
	, params_(dh.dpms())     	
	, wtsetup_(dh.setup())	
	, synthpickset_(dh.pickmgr()->getSynthPickSet())
	, seispickset_(dh.pickmgr()->getSeisPickSet())
	, zrange_(params_->timeintvs_[1])
	, trcbuf_(0)
	, checkshotitm_(0)
    	, seistrcdp_(0)
	, infoMsgChanged(this)	       
	, wellcontrol_(0)
{
    dataholder_.redrawViewerNeeded.notify(
	    			mCB(this,uiTieView,redrawViewerMarkers));
    vwr_->rgbCanvas().scene().getMouseEventHandler().movement.notify( 
	    				mCB( this, uiTieView, setInfoMsg ) );

    initFlatViewer();
    initLogViewers();
    initWellControl();
} 


uiTieView::~uiTieView()
{
    if ( seistrcdp_ )
	removePack();
    dataholder_.redrawViewerNeeded.remove(
	    			mCB(this,uiTieView,redrawViewerMarkers));
    delete trcbuf_;
    deepErase( logsdisp_ );
}


void uiTieView::initWellControl()
{
    mGetWD(return)
    wellcontrol_ = new uiWellDisplayControl( *logsdisp_[0] );
    wellcontrol_->addLogDisplay( *logsdisp_[1] );
    wellcontrol_->posChanged.notify( mCB(this,uiTieView,setInfoMsg) );
}


void uiTieView::fullRedraw()
{
    if ( !setLogsParams() ) 
	return;

    drawVelLog();
    drawDenLog();
    drawAILog();
    drawRefLog();
    drawLogDispWellMarkers();
    drawCShot();
    
    redrawViewer(0);
}


void uiTieView::redrawViewer( CallBacker* cb )
{
    mDynamicCastGet(uiControlView*,ctrl,vwr_->control())
    drawTraces();
    redrawViewerMarkers( cb );
    vwr_->handleChange( FlatView::Viewer::All );    
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
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->setPrefWidth( vwr_->prefHNrPics()/2 );
	logsdisp_[idx]->setPrefHeight( vwr_->prefVNrPics() );
	logsdisp_[idx]->disableScrollZoom();
    }
    logsdisp_[0]->attach( leftOf, logsdisp_[1] );
    logsdisp_[1]->attach( leftOf, vwr_ );
}


void uiTieView::initFlatViewer()
{
    BufferString nm("Synthetics<------------------------------------>Seismics");
    vwr_->setInitialSize( uiSize(520,540) );
    vwr_->setExtraBorders( uiSize(0,0), uiSize(0,20) );
    vwr_->viewChanged.notify( mCB(this,uiTieView,zoomChg) );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.color_ = Color::Black();
    app.annot_.setAxesAnnot( true );
    app.annot_.showaux_ = true ;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.sampling_ = 0.2;
    app.annot_.title_ = nm;
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.right_= Color::Black();
    app.ddpars_.wva_.clipperc_.set(0,0);
    app.ddpars_.wva_.wigg_ = Color::Black();
    app.ddpars_.wva_.overlap_ = 1;
}


bool uiTieView::setLogsParams()
{
    if ( !params_->timeintvs_.size() ) return false;
    const WellTie::Params::uiParams* uipms = dataholder_.uipms();
    mGetWD(return false)
    const Well::D2TModel* d2tm = wd->d2TModel();
    if ( !d2tm ) return false;
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->logData(true).wl_ = 0;
	logsdisp_[idx]->logData(false).wl_ = 0;
	uiWellDahDisplay::Data data;
	data.zrg_ = zrange_;
	data.d2tm_ = d2tm;
	data.markers_ = &wd->markers();
	data.dispzinft_ = uipms->iszinft_;
	data.zistime_ = uipms->iszintime_;
	logsdisp_[idx]->setData( data );
    }
    setLogsRanges( zrange_.start, zrange_.stop );
    return true;
}


void uiTieView::drawVelLog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[0]->logData(true);
    wldld1.wl_ = dataholder_.logset()->getLog( params_->dispcurrvellognm_ );
    wldld1.xrev_ = !wtsetup_.issonic_;
    wldld1.disp_.color_ = Color::stdDrawColor(0);
    wldld1.disp_.isleftfill_ =false;
    wldld1.disp_.isrightfill_ =false;
}


void uiTieView::drawDenLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[0]->logData(false);
    wldld2.wl_ = dataholder_.logset()->getLog( params_->denlognm_ );
    wldld2.xrev_ = false;
    wldld2.disp_.color_ = Color::stdDrawColor(1);
    wldld2.disp_.isleftfill_ =false;
    wldld2.disp_.isrightfill_ =false;
}


void uiTieView::drawAILog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[1]->logData(true);
    wldld1.wl_ = dataholder_.logset()->getLog( params_->ainm_ );
    wldld1.xrev_ = true;
    wldld1.disp_.color_ = Color::stdDrawColor(0);
    wldld1.disp_.isleftfill_ =false;
    wldld1.disp_.isrightfill_ =false;
}


void uiTieView::drawRefLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[1]->logData(false);
    wldld2.wl_ = dataholder_.logset()->getLog( params_->refnm_ );
    wldld2.disp_.color_ = Color::stdDrawColor(1);
    wldld2.disp_.isleftfill_ =false;
    wldld2.disp_.isrightfill_ =false;
}


void uiTieView::drawTraces()
{
    if ( !trcbuf_ )
	trcbuf_ = new SeisTrcBuf(true);
    trcbuf_->erase();

    setUpTrcBuf( trcbuf_, params_->synthnm_, 5 );
    setUpTrcBuf( trcbuf_, params_->seisnm_, 5 );
    setDataPack( trcbuf_, params_->seisnm_, 5 );
}


void uiTieView::setUpTrcBuf( SeisTrcBuf* trcbuf, const char* varname, 
				  int nrtraces )
{
    const int varsz = zrange_.nrSteps();
    SeisTrc valtrc, udftrc;

    valtrc.reSize( varsz, false );
    udftrc.reSize( varsz, false ) ;

    setUpValTrc( valtrc, varname, varsz );
    setUpUdfTrc( udftrc, varname, varsz );

    for ( int idx=0; idx<nrtraces+2; idx++ )
    {
	bool isudf =  ( idx<1 || idx > nrtraces );
	SeisTrc* newtrc = new SeisTrc( isudf? udftrc : valtrc );
	trcbuf->add( newtrc );
	trcbuf->get(trcbuf->size()-1)->info().nr = trcbuf->size()-1;
    }
}


void uiTieView::setUpUdfTrc( SeisTrc& trc, const char* varname, int varsz )
{
    for ( int idx=0; idx<varsz; idx++)
	trc.set( idx, mUdf(float), 0 );
}


void uiTieView::setUpValTrc( SeisTrc& trc, const char* varname, int varsz )
{
    Array1DImpl<float> vals = *dataholder_.getLogVal( varname );
    if ( !vals.info().getSize(0) ) return;
    for ( int idx=0; idx<varsz; idx++)
    {
	float val = vals.get( idx );
	if ( mIsUdf(val) )
	    val = 0;
	trc.set( idx, val, 0 );
	trc.info().sampling.start = zrange_.start *1000;
	trc.info().sampling.step = zrange_.step * 1000;
    }
    SeisTrcPropChg pc( trc );
    pc.normalize( true ); 
}


void uiTieView::setDataPack( SeisTrcBuf* trcbuf, const char* varname, 
				 int vwrnr )
{ 
    if ( seistrcdp_ )
    { removePack(); seistrcdp_=0; }

    seistrcdp_ = new SeisTrcBufDataPack( trcbuf, Seis::Vol, 
				SeisTrcInfo::TrcNr, "Seismic" );
    seistrcdp_->setName( varname );
    seistrcdp_->trcBufArr2D().setBufMine( false );

    StepInterval<double> xrange( 1, trcbuf->size(), 1 );
    seistrcdp_->posData().setRange( true, xrange );
    StepInterval<double> zrange( zrange_.start*1000, 
	    			 zrange_.stop *1000, 
	   			 zrange_.step *1000 );
    seistrcdp_->posData().setRange( false, zrange );
    seistrcdp_->setName( varname );
    
    DPM(DataPackMgr::FlatID()).add( seistrcdp_ );
    FlatView::Appearance& app = vwr_->appearance();
    vwr_->setPack( true, seistrcdp_->id(), false, true );

    const UnitOfMeasure* uom = 0;
    const char* units =  ""; //uom ? uom->symbol() : "";
    app.annot_.x1_.name_ =  varname;
    app.annot_.x2_.name_ = "TWT (ms)";
}


void uiTieView::setLogsRanges( float start, float stop )
{
    Interval<float> rg( start, stop ); 
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( rg );
}


void uiTieView::removePack()
{
    if ( seistrcdp_ ) DPM( DataPackMgr::FlatID() ).release( seistrcdp_->id() );
}


void uiTieView::zoomChg( CallBacker* )
{
    mGetWD(return)
    const Well::D2TModel* d2tm = wd->d2TModel();
    if ( !d2tm ) return; 

    uiWorldRect curwr = vwr_->curView();
    const float start = curwr.top();
    const float stop  = curwr.bottom();
    setLogsRanges( start, stop );
    drawCShot();
}


void uiTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
			    bool left, float zpos, Color col, bool ispick )
{
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.width_ = ispick ? 2 : 1;
    auxdata->linestyle_.type_  = ispick ? LineStyle::Solid : LineStyle::Dot;
    
    const float xleft = vwr_->boundingBox().left();
    const float xright = vwr_->boundingBox().right();
    bool isleft = true;

    auxdata->poly_ += FlatView::Point( left ? (xright-xleft)/2 : xright, zpos );
    auxdata->poly_ += FlatView::Point( left ? xleft : (xright-xleft)/2, zpos );

    if ( !ispick  )
    {
	BufferString mtxt( auxdata->name_ );
	if ( mtxt.size() > 3 )
	    mtxt[3] = '\0';
	auxdata->name_ = mtxt;
	auxdata->namealignment_ = Alignment(Alignment::HCenter,Alignment::Top);
	auxdata->namepos_ = 1;
    }
}	


void uiTieView::drawLogDispWellMarkers()
{
    Well::Data* wd = dataholder_.wd();
    if ( !wd ) return;
    bool ismarkerdisp = dataholder_.uipms()->ismarkerdisp_;
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
    Well::Data* wd = dataholder_.wd();
    if ( !wd ) return;

    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( wellmarkerauxdatas_ );
    bool ismarkerdisp = dataholder_.uipms()->isvwrmarkerdisp_;
    if ( !ismarkerdisp ) 
	return;
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
	
	drawMarker( auxdata, true, zpos*1000, col, false );
    }
}	


void uiTieView::drawUserPicks()
{
    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( userpickauxdatas_ );
    const int nrauxs = mMAX( seispickset_->getSize(),synthpickset_->getSize());
    
    for ( int idx=0; idx<nrauxs; idx++ )
    {
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
	userpickauxdatas_ += auxdata;
	app.annot_.auxdata_ +=  auxdata;
    }
    
    drawUserPicks( seispickset_ );
    drawUserPicks( synthpickset_ );

    vwr_->handleChange( FlatView::Viewer::Annot );    
}


void uiTieView::drawUserPicks( const WellTie::PickSet* pickset )
{
    for ( int idx=0; idx<pickset->getSize(); idx++ )
    {
	const UserPick* pick = pickset->get(idx);
	if ( !pick  ) continue;

	float zpos = pick->zpos_*1000; 
	float xpos = pick->xpos_;
	
	drawMarker( userpickauxdatas_[idx], pick->issynthetic_, 
		    zpos, pick->color_, true );
    }
}


void uiTieView::drawHorizons()
{
    mRemoveItms( hortxtnms_ )
    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( horauxdatas_ );
    bool ishordisp = dataholder_.uipms()->isvwrhordisp_;
    if ( !ishordisp ) return;
    const ObjectSet<DataHolder::HorData>& hordatas = dataholder_.horDatas();
    for ( int idx=0; idx<hordatas.size(); idx++ )
    {
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
	horauxdatas_ += auxdata;
	app.annot_.auxdata_ += auxdata;
	float zval = hordatas[idx]->zval_;
	auxdata->name_ = hordatas[idx]->name_;
	drawMarker( auxdata, false, zval, hordatas[idx]->color_, false );
    }
}


void uiTieView::drawCShot()
{
    uiGraphicsScene& scene = logsdisp_[0]->scene();
    scene.removeItem( checkshotitm_ );
    delete checkshotitm_; checkshotitm_=0;
    if ( !dataholder_.uipms()->iscsdisp_ ) 
	return;

    mGetWD(return)
    const Well::D2TModel* cs = wd->checkShotModel();
    if ( !cs  ) return;
    const int sz = cs->size();
    if ( sz < 2 ) return;
    
    WellTie::GeoCalculator geocalc( dataholder_ );

    TypeSet<float> cstolog;
    geocalc.checkShot2Log( cs, wtsetup_.issonic_, cstolog );
    
    TypeSet<uiPoint> pts;
    uiWellLogDisplay::LogData& ld = logsdisp_[0]->logData(true);
    Interval<float> zrg = ld.zrg_;

    const bool dispintime = dataholder_.uipms()->iszintime_;
    const Well::D2TModel* d2tm = wd->d2TModel();
    if ( dispintime && !d2tm ) return; 

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = cstolog[idx];
	float zpos = cs->dah(idx);
	if ( dispintime ) 
	    zpos = d2tm->getTime( zpos )*1000;
	if ( zpos < zrg.start )
	    continue;
	else if ( zpos > zrg.stop )
	    break;
	
	if ( dataholder_.uipms()->iszinft_ ) zpos *= mToFeetFactor;
	pts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(zpos) );
    }
    if ( pts.isEmpty() ) return;

    checkshotitm_ = scene.addItem( new uiPolyLineItem(pts) );
    LineStyle ls( LineStyle::Solid, 2, Color::DgbColor() );
    checkshotitm_->setPenStyle( ls );
    //logsdisp_[0]->doDataChange();
}


bool uiTieView::isEmpty()
{
    return dataholder_.logset()->isEmpty();
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


uiCorrView::uiCorrView( uiParent* p, WellTie::DataHolder& dh)
	: uiGroup(p)
    	, dataholder_(dh)  
{
    uiFunctionDisplay::Setup fdsu; 
    fdsu.border_.setLeft( 2 );		fdsu.border_.setRight( 0 );
    fdsu.epsaroundzero_ = 1e-3;

    for (int idx=0; idx<1; idx++)
    {
	corrdisps_ += new uiFunctionDisplay( this, fdsu );
	corrdisps_[idx]->xAxis()->setName( "Lags (ms)" );
	corrdisps_[idx]->yAxis(false)->setName( "Coefficient" );
    }
    	
    corrlbl_ = new uiLabel( this,"" );
    corrlbl_->attach( centeredAbove, corrdisps_[0] );
}


uiCorrView::~uiCorrView()
{
    deepErase( corrdisps_ );
}


void uiCorrView::setCrossCorrelation()
{
    const WellTie::Params::DataParams& params = *dataholder_.dpms(); 
    Array1DImpl<float> corrarr = *dataholder_.getLogVal( params.crosscorrnm_ );
    const int datasz = corrarr.info().getSize(0);
    if ( !datasz ) return;
    
    const float normalfactor = dataholder_.corrcoeff() / corrarr.get(datasz/2);
    TypeSet<float> xvals,corrvals;
    for ( int idx=-datasz/2; idx<datasz/2; idx++)
    {
	float xaxistime = idx*params.timeintvs_[2].step*1000;
	if ( fabs( xaxistime ) > 200  )
	    continue;
	xvals += xaxistime;
	float val = corrarr.get(idx+datasz/2);
	val *= normalfactor;
	corrvals += fabs(val)>1 ? 0 : val;
    }

    for (int idx=0; idx<corrdisps_.size(); idx++)
	corrdisps_[idx]->setVals( xvals.arr(), corrvals.arr(), xvals.size() );
    
    BufferString corrbuf = "Cross-Correlation Coefficient: ";
    corrbuf += dataholder_.corrcoeff();
    corrlbl_->setPrefWidthInChar(50);
    corrlbl_->setText( corrbuf );
}

}; //namespace WellTie
