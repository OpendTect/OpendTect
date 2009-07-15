/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.36 2009-07-15 09:13:41 cvsbruno Exp $";

#include "uiwelltieview.h"

#include "uiaxishandler.h"
#include "uidialog.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uitabstack.h"
#include "uiwelllogdisplay.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "linear.h"
#include "flatposdata.h"
#include "geometry.h"
#include "iostrm.h"
#include "unitofmeasure.h"
#include "posinfo.h"
#include "position.h"
#include "posvecdataset.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbufadapters.h"
#include "survinfo.h"
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



uiWellTieView::uiWellTieView( uiParent* p, uiFlatViewer* vwr, 
			      WellTieDataHolder* dhr,
			      ObjectSet<uiWellLogDisplay>* ldis )  
	: wd_(*dhr->wd()) 
	, vwr_(vwr)  
	, logsdisp_(*ldis)	     
	, dataholder_(dhr)  
	, params_(dhr->dpms())     	
	, wtsetup_(dhr->setup())	
    	, data_(*dhr->dispData())
	, datamgr_(*dhr->datamgr())
	, synthpickset_(dhr->pickmgr()->getSynthPickSet())
	, seispickset_(dhr->pickmgr()->getSeisPickSet())
	, trcbuf_(0)
	, checkshotitm_(0)
    	, seistrcdp_(0)
{
    initFlatViewer();
    initLogViewers();
} 


uiWellTieView::~uiWellTieView()
{
    if ( seistrcdp_ )
	removePack();
    delete trcbuf_;
}


void uiWellTieView::fullRedraw()
{
    setLogsParams();
    drawVelLog();
    drawDenLog();
    drawAILog();
    drawRefLog();
    drawTraces();
    drawWellMarkers();
    drawUserPicks();
    drawCShot();
    for ( int idx =0; idx<logsdisp_.size(); idx++ )
	logsdisp_[idx]->dataChanged();
    vwr_->handleChange( FlatView::Viewer::Annot );    
}



void uiWellTieView::initLogViewers()
{
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->setPrefWidth( vwr_->prefHNrPics()/2 );
	logsdisp_[idx]->setPrefHeight( vwr_->prefVNrPics() );
    }
    logsdisp_[0]->attach( leftOf, logsdisp_[1] );
    logsdisp_[1]->attach( leftOf, vwr_ );
}


void uiWellTieView::initFlatViewer()
{
    BufferString nm("Synthetics<------------------------------------>Seismics");
    vwr_->setInitialSize( uiSize(490,540) );
    vwr_->viewChanged.notify( mCB(this,uiWellTieView,zoomChg) );
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
    app.ddpars_.wva_.right_= Color(0,0,255);
    app.ddpars_.wva_.left_= Color(255,0,0);
    app.ddpars_.wva_.clipperc_.set(0,0);
    app.ddpars_.wva_.wigg_ = Color::Black();
    app.ddpars_.wva_.overlap_ = 1;
}


void uiWellTieView::setLogsParams()
{
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
	logsdisp_[idx]->setZDispInFeet( dataholder_->uipms()->iszinft_ );
    setLogsRanges( params_->dptintv_.start, params_->dptintv_.stop );
}


void uiWellTieView::drawVelLog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[0]->logData( true );
    wldld1.wl_ = wd_.logs().getLog( params_->dispcurrvellognm_ );
    wldld1.xrev_ = !wtsetup_.issonic_;
    wldld1.linestyle_.color_ = Color::stdDrawColor(0);
}


void uiWellTieView::drawDenLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[0]->logData( false );
    wldld2.wl_ = wd_.logs().getLog( params_->denlognm_ );
    wldld2.linestyle_.color_ = Color::stdDrawColor(1);
}


void uiWellTieView::drawAILog()
{
    uiWellLogDisplay::LogData& wldld1 = logsdisp_[1]->logData( true );
    wldld1.wl_ = wd_.logs().getLog( params_->ainm_ );
    //wldld2.xrev_ = true;
    wldld1.linestyle_.color_ = Color::stdDrawColor(0);
}


void uiWellTieView::drawRefLog()
{
    uiWellLogDisplay::LogData& wldld2 = logsdisp_[1]->logData( false );
    wldld2.wl_ = wd_.logs().getLog( params_->refnm_ );
    wldld2.linestyle_.color_ = Color::stdDrawColor(1);
}


void uiWellTieView::drawTraces()
{
    if ( !trcbuf_ )
	trcbuf_ = new SeisTrcBuf(true);
    trcbuf_->erase();

    setUpTrcBuf( trcbuf_, params_->synthnm_, 5 );
    setUpTrcBuf( trcbuf_, params_->attrnm_, 5 );
    setDataPack( trcbuf_, params_->attrnm_, 5 );
}


void uiWellTieView::setUpTrcBuf( SeisTrcBuf* trcbuf, const char* varname, 
				  int nrtraces )
{
    const int varsz = data_.getLength();
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


void uiWellTieView::setUpUdfTrc( SeisTrc& trc, const char* varname, int varsz )
{
    for ( int idx=0; idx<varsz; idx++)
	trc.set( idx, mUdf(float), 0 );
}


void uiWellTieView::setUpValTrc( SeisTrc& trc, const char* varname, int varsz )
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


void uiWellTieView::setDataPack( SeisTrcBuf* trcbuf, const char* varname, 
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

    vwr_->handleChange( FlatView::Viewer::All );
    const UnitOfMeasure* uom = 0;
    const char* units =  ""; //uom ? uom->symbol() : "";
    app.annot_.x1_.name_ =  varname;
    app.annot_.x2_.name_ = "TWT (ms)";
}


void uiWellTieView::setLogsRanges( float start, float stop )
{
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( Interval<float>( start, stop) );
}


void uiWellTieView::removePack()
{
    if ( seistrcdp_ ) DPM( DataPackMgr::FlatID() ).release( seistrcdp_->id() );
}


void uiWellTieView::zoomChg( CallBacker* )
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    uiWorldRect curwr = vwr_->curView();
    const float start = curwr.top();
    const float stop  = curwr.bottom();
    setLogsRanges( d2tm->getDepth(start*0.001), d2tm->getDepth(stop*0.001) );
    drawCShot();
}


void uiWellTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
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


void uiWellTieView::drawWellMarkers()
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
    bool ismarkerdisp = dataholder_->uipms()->ismarkerdisp_;
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
	logsdisp_[idx]->setMarkers( ismarkerdisp ? &wd_.markers() : 0 );
}	

#define mRemoveSet( auxs ) \
    for ( int idx=0; idx<auxs.size(); idx++ ) \
        app.annot_.auxdata_ -= auxs[idx]; \
    deepErase( auxs );
void uiWellTieView::drawUserPicks()
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


void uiWellTieView::drawUserPicks( const WellTiePickSet* pickset )
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


void uiWellTieView::drawCShot()
{
    uiGraphicsScene& scene = logsdisp_[0]->scene();
    scene.removeItem( checkshotitm_ );
    delete checkshotitm_; checkshotitm_=0;
    if ( !dataholder_->uipms()->iscsdisp_ ) 
	return;
    const Well::D2TModel* cs = wd_.checkShotModel();
    if ( !cs  ) return;
    const int sz = cs->size();
    
    WellTieGeoCalculator geocalc( dataholder_->params(), &wd_ );

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

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = cstolog[idx];
	float dah = dpt[idx];
	if ( dah < zrg.start )
	    continue;
	else if ( dah > zrg.stop )
	    break;
	
	if ( dataholder_->uipms()->iszinft_ ) dah *= mToFeetFactor;
	pts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(dah) );
    }
    if ( pts.isEmpty() ) return;

    checkshotitm_ = scene.addItem( new uiPolyLineItem(pts) );
    LineStyle ls( LineStyle::Solid, 2, Color::DgbColor() );
    checkshotitm_->setPenStyle( ls );
}




uiWellTieCorrView::uiWellTieCorrView( uiParent* p, WellTieDataHolder* dh)
	: uiGroup(p)
    	, params_(*dh->dpms())  
	, corrdata_(*dh->corrData())
	, welltiedata_(dh->data())			
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


uiWellTieCorrView::~uiWellTieCorrView()
{
    deepErase( corrdisps_ );
}


void uiWellTieCorrView::setCrossCorrelation()
{
    const int datasz = corrdata_.get(params_.crosscorrnm_)->info().getSize(0);
    
    const float corrcoeff = welltiedata_.corrcoeff_; 
    float normalfactor = corrcoeff/corrdata_.get(params_.crosscorrnm_,datasz/2);
    TypeSet<float> xvals,corrvals;
    for ( int idx=-datasz/2; idx<datasz/2; idx++)
    {
	float xaxistime = idx*params_.timeintv_.step*params_.step_*1000;
	if ( fabs( xaxistime ) > 200  )
	    continue;
	xvals += xaxistime;
	float val = corrdata_.get(params_.crosscorrnm_, idx+datasz/2);
	val *= normalfactor;
	if ( fabs(val)>1 )
	    corrvals += 0;
	else
	    corrvals += val;
    }


    for (int idx=0; idx<corrdisps_.size(); idx++)
	corrdisps_[idx]->setVals( xvals.arr(), corrvals.arr(), xvals.size() );
    
    BufferString corrbuf = "Cross-Correlation Coefficient: ";
    corrbuf += corrcoeff;
    corrlbl_->setPrefWidthInChar(50);
    corrlbl_->setText( corrbuf );
}

