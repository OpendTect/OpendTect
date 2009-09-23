/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.44 2009-09-23 11:50:08 cvsbruno Exp $";

#include "uiwelltieview.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiwelllogdisplay.h"

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


namespace WellTie
{

uiTieView::uiTieView( uiParent* p, uiFlatViewer* vwr, 
		      WellTie::DataHolder& dh,
		      ObjectSet<uiWellLogDisplay>* ldis )  
	: wd_(*dh.wd()) 
	, vwr_(vwr)  
	, logsdisp_(*ldis)	     
	, dataholder_(dh)  
	, params_(dh.dpms())     	
	, wtsetup_(dh.setup())	
    	, data_(*dh.logsset())
	, synthpickset_(dh.pickmgr()->getSynthPickSet())
	, seispickset_(dh.pickmgr()->getSeisPickSet())
	, trcbuf_(0)
	, checkshotitm_(0)
    	, seistrcdp_(0)
{
    initFlatViewer();
    initLogViewers();
} 


uiTieView::~uiTieView()
{
    if ( seistrcdp_ )
	removePack();
    delete trcbuf_;
}


void uiTieView::fullRedraw()
{
    setLogsParams();
    drawVelLog();
    drawDenLog();
    drawAILog();
    drawRefLog();
    drawWellMarkers();
    drawCShot();
    for ( int idx =0; idx<logsdisp_.isEmpty(); idx++ )
	logsdisp_[idx]->dataChanged();
    redrawViewer();
}


void uiTieView::redrawViewer()
{
    drawTraces();
    drawUserPicks();
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
    vwr_->setInitialSize( uiSize(490,540) );
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


void uiTieView::setLogsParams()
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return;
    const WellTie::Params::uiParams* uipms = dataholder_.uipms();
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->setD2TModel( d2tm );
	logsdisp_[idx]->setZDispInFeet( uipms->iszinft_ );
	logsdisp_[idx]->setZInTime( uipms->iszintime_ );
    }
    setLogsRanges( params_->dptintv_.start, params_->dptintv_.stop );

}


void uiTieView::drawVelLog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[0]->logData( true );
    wldld1.wl_ = data_.getLog( params_->dispcurrvellognm_ );
    wldld1.xrev_ = !wtsetup_.issonic_;
    wldld1.linestyle_.color_ = Color::stdDrawColor(0);
}


void uiTieView::drawDenLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[0]->logData( false );
    wldld2.wl_ = data_.getLog( params_->denlognm_ );
    wldld2.linestyle_.color_ = Color::stdDrawColor(1);
}


void uiTieView::drawAILog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[1]->logData( true );
    wldld1.wl_ = data_.getLog( params_->ainm_ );
    wldld1.linestyle_.color_ = Color::stdDrawColor(0);
}


void uiTieView::drawRefLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[1]->logData( false );
    wldld2.wl_ = data_.getLog( params_->refnm_ );
    wldld2.linestyle_.color_ = Color::stdDrawColor(1);
}


void uiTieView::drawTraces()
{
    if ( !trcbuf_ )
	trcbuf_ = new SeisTrcBuf(true);
    trcbuf_->erase();

    setUpTrcBuf( trcbuf_, params_->synthnm_, 5 );
    setUpTrcBuf( trcbuf_, params_->attrnm_, 5 );
    setDataPack( trcbuf_, params_->attrnm_, 5 );
}


void uiTieView::setUpTrcBuf( SeisTrcBuf* trcbuf, const char* varname, 
				  int nrtraces )
{
    const int varsz = params_->dispsize_;
    SeisTrc valtrc;
    SeisTrc udftrc;

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
    for ( int idx=0; idx<varsz; idx++)
    {
	float val = data_.get( varname, idx );
	if ( mIsUdf(val) )
	    val = 0;
	trc.set( idx, val, 0 );
    }
    SeisTrcPropChg pc( trc );
    pc.normalize( true ); 
}


void uiTieView::setDataPack( SeisTrcBuf* trcbuf, const char* varname, 
				 int vwrnr )
{ 
    if ( seistrcdp_ )
    { removePack(); seistrcdp_=0; }

    const int type = trcbuf->get(0)->info().getDefaultAxisFld( 
			    Seis::Line, &trcbuf->get(1)->info() );
    seistrcdp_ =
	new SeisTrcBufDataPack( trcbuf, Seis::Line, 
				(SeisTrcInfo::Fld)type, "Seismic" );
    seistrcdp_->setName( varname );
    seistrcdp_->trcBufArr2D().setBufMine( false );

    DPM(DataPackMgr::FlatID()).addAndObtain( seistrcdp_ );
    StepInterval<double> xrange( 1, trcbuf->size(), 1 );
    seistrcdp_->posData().setRange( true, xrange );
    seistrcdp_->setName( varname );
    
    FlatView::Appearance& app = vwr_->appearance();
    vwr_->setPack( true, seistrcdp_->id(), false, true );

    const UnitOfMeasure* uom = 0;
    const char* units =  ""; //uom ? uom->symbol() : "";
    app.annot_.x1_.name_ =  varname;
    app.annot_.x2_.name_ = "TWT (ms)";
}


void uiTieView::setLogsRanges( float start, float stop )
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 
    if ( dataholder_.uipms()->iszintime_ )
    {
	start = d2tm->getTime( start )*1000;
	stop = d2tm->getTime( stop )*1000;
    }
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( Interval<float>( start, stop) );
}


void uiTieView::removePack()
{
    if ( seistrcdp_ ) DPM( DataPackMgr::FlatID() ).release( seistrcdp_->id() );
}


void uiTieView::zoomChg( CallBacker* )
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    uiWorldRect curwr = vwr_->curView();
    const float start = curwr.top();
    const float stop  = curwr.bottom();
    setLogsRanges( d2tm->getDepth(start*0.001), d2tm->getDepth(stop*0.001) );
    drawCShot();
}


void uiTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
       				int vwridx, float xpos,  float zpos,
			       	Color col, bool ispick )
{
    FlatView::Appearance& app = vwr_->appearance();
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.width_ = 2;
    auxdata->linestyle_.type_  = LineStyle::Solid;
    
    const float xleft = vwr_->boundingBox().left();
    const float xright = vwr_->boundingBox().right();

    if ( xpos && xpos < ( xright-xleft )/2 )
    {
	auxdata->poly_ += FlatView::Point( (xright-xleft)/2, zpos );
	auxdata->poly_ += FlatView::Point( xleft, zpos );
    }
    else if ( xpos && xpos>( xright-xleft )/2 )
    {
	auxdata->poly_ += FlatView::Point( xright, zpos );
	auxdata->poly_ += FlatView::Point( (xright-xleft)/2, zpos );
    }
}	


void uiTieView::drawWellMarkers()
{
    deepErase( wellmarkerauxdatas_ );
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 
   
    for ( int midx=0; midx<wd_.markers().size(); midx++ )
    {
	Well::Marker* marker = const_cast<Well::Marker*>( 
						wd_.markers()[midx] );
	if ( !marker  ) continue;
	
	float zpos = d2tm->getTime( marker->dah() ); 
	const Color col = marker->color();
	
	if ( zpos < params_->timeintv_.start || zpos > params_->timeintv_.stop 
		|| col == Color::NoColor() || col.rgb() == 16777215 )
	    continue;
	
	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(marker->name()) );
	wellmarkerauxdatas_ += auxdata;
	
	//drawMarker( auxdata, 0, 0, zpos, col, false );
    }
    bool ismarkerdisp = dataholder_.uipms()->ismarkerdisp_;
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
	logsdisp_[idx]->setMarkers( ismarkerdisp ? &wd_.markers() : 0 );
}	

#define mRemoveSet( auxs ) \
    for ( int idx=0; idx<auxs.size(); idx++ ) \
        app.annot_.auxdata_ -= auxs[idx]; \
    deepErase( auxs );
void uiTieView::drawUserPicks()
{
    FlatView::Appearance& app = vwr_->appearance();
    mRemoveSet( userpickauxdatas_ );
    const int nrauxs = mMAX( seispickset_->getSize(),synthpickset_->getSize() );
    
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
	
	drawMarker( userpickauxdatas_[idx], 0, xpos, zpos, 
		    pick->color_, false );
    }
}


void uiTieView::drawCShot()
{
    uiGraphicsScene& scene = logsdisp_[0]->scene();
    scene.removeItem( checkshotitm_ );
    delete checkshotitm_; checkshotitm_=0;
    if ( !dataholder_.uipms()->iscsdisp_ ) 
	return;
    const Well::D2TModel* cs = wd_.checkShotModel();
    if ( !cs  ) return;
    const int sz = cs->size();
    if ( sz < 2 ) return;
    
    WellTie::GeoCalculator geocalc( dataholder_ );

    TypeSet<float> csvals, cstolog, dpt;
    for ( int idx=0; idx<sz; idx++ )
    {
	float val = cs->value( idx );
	float dah = cs->dah( idx );
	csvals += val;
	dpt    += dah;
    }
    geocalc.TWT2Vel( csvals, dpt, cstolog, true );
    
    TypeSet<uiPoint> pts;
    uiWellLogDisplay::LogData& ld = logsdisp_[0]->logData();
    Interval<float> zrg = ld.zrg_;

    const bool dispintime = dataholder_.uipms()->iszintime_;
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( dispintime && !d2tm ) return; 

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = cstolog[idx];
	float zpos = dpt[idx];
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
    logsdisp_[0]->dataChanged();
}




uiCorrView::uiCorrView( uiParent* p, const WellTie::DataHolder& dh)
	: uiGroup(p)
    	, dataholder_(dh)  
	, data_(*dh.logsset())
{
    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );

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
    const int datasz = data_.getLog(params.crosscorrnm_)->size();
    
    const float normalfactor = dataholder_.corrcoeff()
			     / data_.get(params.crosscorrnm_,datasz/2);
    TypeSet<float> xvals,corrvals;
    for ( int idx=-datasz/2; idx<datasz/2; idx++)
    {
	float xaxistime = idx*params.timeintv_.step*params.step_*1000;
	if ( fabs( xaxistime ) > 200  )
	    continue;
	xvals += xaxistime;
	float val = data_.get(params.crosscorrnm_, idx+datasz/2);
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
