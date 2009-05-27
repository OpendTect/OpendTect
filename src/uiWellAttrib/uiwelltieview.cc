/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.11 2009-05-27 04:35:19 cvsnanne Exp $";

#include "uiwelltieview.h"

#include "uiflatviewer.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "iostrm.h"
#include "geometry.h"
#include "unitofmeasure.h"
#include "posinfo.h"
#include "position.h"
#include "posvecdataset.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welltiesetup.h"
#include "welltiedata.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"
#include "welltiepickset.h"

uiWellTieView::uiWellTieView( uiParent* p, WellTieDataMGR& mgr,  
			      const Well::Data* d, const  WellTieParams* pm )
	: wd_(*d)  
	, params_(*pm)     	
	, wtsetup_(pm->getSetup())	
	, datamgr_(mgr)
    	, data_(*mgr.getDispData())
{
    createViewers( (uiGroup*)p );
} 


uiWellTieView::~uiWellTieView()
{
    deleteWellMarkers();
    deleteUserPicks();
    deleteCheckShot();

    for (int vwridx=vwrs_.size()-1; vwridx>=0; vwridx--)
	vwrs_.remove(vwridx);
}


void uiWellTieView::fullRedraw()
{
    drawVelLog();
    drawDenLog();
    drawAI();
    drawReflectivity();
    drawSynthetics();
    drawSeismic();
    drawWellMarkers();
    drawUserPicks(0);
    drawCShot();
}


void uiWellTieView::createViewers( uiGroup* vwrgrp )
{
    for (int vwridx=0; vwridx<6; vwridx++)
    {
	vwrs_ += new uiFlatViewer( vwrgrp );
	if ( vwridx>0 )
	    vwrs_[vwridx]->attach( rightOf, vwrs_[vwridx-1] );
    }

    initFlatViewer( wtsetup_.vellognm_, 0, 200, 550, false, Color::Black() );
    initFlatViewer( wtsetup_.denlognm_, 1, 200, 550, false, Color::Black() );
    initFlatViewer( params_.ainm_, 2, 200, 550, false, Color::Black() );
    initFlatViewer( params_.refnm_, 3, 200, 550, false, Color::Black() );
    initFlatViewer( params_.synthnm_, 4, 200, 550, true, Color(0,0,255) );
    initFlatViewer( "Seismics", 5, 200, 550, true, Color(255,0,0) );
}


//TODO move to display properties
void uiWellTieView::initFlatViewer( const char* nm, int nr, int xsize,
				    int ysize, bool iswigg, const Color& col)
{
    vwrs_[nr]->setInitialSize( uiSize(xsize,ysize) );
    FlatView::Appearance& app = vwrs_[nr]->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.color_ = Color::Black();
    app.annot_.setAxesAnnot( true );
    app.annot_.showaux_ = true ;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.sampling_ = 0.2;
    app.annot_.title_ = nm;
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.right_= iswigg? col : Color::NoColor() ;
    app.ddpars_.wva_.clipperc_.set(0,0);
    app.ddpars_.wva_.wigg_ = col;
}


void uiWellTieView::drawVelLog()
{
    createVarDataPack( params_.currvellognm_, 0, 1);
}


void uiWellTieView::drawDenLog()
{
    createVarDataPack( wtsetup_.denlognm_, 1, 1 );
}


void uiWellTieView::drawAI()
{
    createVarDataPack( params_.ainm_, 2, 1 );
}


void uiWellTieView::drawReflectivity()
{
    createVarDataPack( params_.refnm_, 3, 1 );
}


void uiWellTieView::drawSynthetics()
{
    createVarDataPack( params_.synthnm_, 4, 3 );
}


void uiWellTieView::drawSeismic()
{
    createVarDataPack( params_.attrnm_, 5, 3 );
}


void uiWellTieView::createVarDataPack( const char* varname, int vwrnr, 
					int nrtraces )
{
    const int varsz = data_.getLength();
    float maxval = data_.getExtremVal( varname, true );
    float minval = data_.getExtremVal( varname, false );
    
    Array2DImpl<float>*  arr2d = new Array2DImpl<float>( nrtraces, varsz );

    for ( int idz=0; idz<varsz; idz++)
    {
	float val =  data_.get( varname, idz );
	for ( int idx=0; idx<nrtraces; idx++)
	    arr2d->set( idx, idz, val );
    }
    vwrs_[vwrnr]->removePack(0);
    FlatView::Appearance& app = vwrs_[vwrnr]->appearance();
    app.ddpars_.wva_.overlap_ = nrtraces > 1 ? 3: maxval-minval-1;
    
    const float shift =  minval + ( maxval - minval )/2;
    StepInterval<double> xrange( shift, shift, maxval-minval);
    StepInterval<double> zrange( params_.timeintv_.start, 
	    			 params_.timeintv_.stop,
				 params_.timeintv_.step*params_.step_ );
    FlatDataPack* dp = new FlatDataPack( "", arr2d );
    DPM(DataPackMgr::FlatID()).add( dp );
    if ( nrtraces < 2 )
	dp->posData().setRange( true, xrange );
    dp->posData().setRange( false, zrange );
    dp->setName( varname );
    vwrs_[vwrnr]->setPack( true, dp->id(), false, true );
    const UnitOfMeasure* uom = 0;
    const char* units =  ""; //uom ? uom->symbol() : "";
    app.annot_.x1_.name_ =  units;
    app.annot_.x2_.name_ = "TWT (ms)";
    vwrs_[vwrnr]->handleChange( FlatView::Viewer::Annot );
}


void uiWellTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
       				int vwridx, float xpos,  float zpos,
			       	Color col, bool ispick )
{
    FlatView::Appearance& app = vwrs_[vwridx]->appearance();
    app.annot_.auxdata_ +=  auxdata;
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.type_  = LineStyle::Solid;

    if ( ispick )
    {
	auxdata->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Cross,5,col );
	Geom::PosRectangle<double> boundingdata( xpos, 0, xpos, 0  );
	auxdata->poly_ += FlatView::Point( boundingdata.left(), zpos);
    }
    else
    {
	auxdata->poly_ += FlatView::Point(
		vwrs_[vwridx]->boundingBox().left(), zpos );
	auxdata->poly_ += FlatView::Point(
		vwrs_[vwridx]->boundingBox().right(), zpos );
    }
}


void uiWellTieView::drawWellMarkers()
{
    bool ismarkergrdline = true;
    int vwrsz = ismarkergrdline ? vwrs_.size() : 1;
  
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 
    
    deleteWellMarkers();

    for ( int midx=0; midx<wd_.markers().size(); midx++ )
    {
	const Well::Marker* marker = wd_.markers()[midx];
	if ( !marker  ) continue;
	
	float zpos = d2tm->getTime( marker->dah() ); 
	const Color col = marker->color();
	
	if ( zpos < params_.timeintv_.start || zpos > params_.timeintv_.stop ||
		    col == Color::NoColor() || col.rgb() == 16777215 )
	return;

	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(marker->name()) );
	wellmarkerauxdatas_ += auxdata;
	
	for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	    drawMarker( auxdata, vwridx, 0,  zpos, col, false );
    }
    for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}	


void uiWellTieView::drawUserPicks( const WellTiePickSet* pickset )
{
    deleteUserPicks();
    if ( !pickset) 
	return;

    const int vwridx = pickset->get(0)->vidx_;
    
    for ( int idx=0; idx<pickset->getSize(); idx++ )
    {
	const UserPick* userpick = pickset->get(idx);
	if ( !userpick  ) continue;

	FlatView::Annotation::AuxData* auxdata = 0;
	mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
	userpickauxdatas_ += auxdata;
	
	float zpos = userpick->zpos_;
	const float xpos = data_.get( params_.currvellognm_,
		 		      data_.getIdx( zpos ) );

	bool isxpick = vwridx? false : true;
	drawMarker( auxdata, vwridx, xpos, zpos, userpick->color_, isxpick );
    }	
    vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}


void uiWellTieView::drawCShot()
{
    deleteCheckShot();

    if ( !params_.iscsdisp_ )
	return;
    
    const Well::D2TModel* cs = wd_.checkShotModel();
    if ( !cs  ) return;
    WellTieGeoCalculator geocalc( &params_, &wd_ );

    TypeSet<float> csvals, cstolog, dpt;
    for ( int idx=0; idx<cs->size(); idx++ )
    {
	dpt     += cs->dah(idx);
	csvals +=  cs->value(idx);
    }
    geocalc.TWT2Vel( csvals, dpt, cstolog, true );

    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    FlatView::Appearance& app = vwrs_[0]->appearance();
    
    FlatView::Annotation::AuxData* auxdata = 0;
    mTryAlloc( auxdata, FlatView::Annotation::AuxData(0) );
    app.annot_.auxdata_ +=  auxdata;
    csauxdatas_ += auxdata;
   
    for ( int idx=0; idx<cs->size(); idx++ )
    {
	auxdata->linestyle_.color_ = Color::DgbColor();
	auxdata->linestyle_.type_  = LineStyle::Solid;
	float zpos = d2tm->getTime( cs->dah(idx) ); 
	auxdata->poly_ += FlatView::Point( cstolog[idx], zpos );
    }
    vwrs_[0]->handleChange( FlatView::Viewer::Annot );
}


void uiWellTieView::deleteWellMarkers()
{
    deleteMarkerAuxDatas( wellmarkerauxdatas_ );
}


void uiWellTieView::deleteUserPicks()
{
    deleteMarkerAuxDatas( userpickauxdatas_  );
}


void uiWellTieView::deleteCheckShot()
{
    deleteMarkerAuxDatas( csauxdatas_ );
}


void uiWellTieView::deleteMarkerAuxDatas(   
			    ObjectSet<FlatView::Annotation::AuxData>& auxset )
{
    for (int midx=auxset.size()-1; midx>=0; midx--)
    {
	for (int vidx=0; vidx<vwrs_.size(); vidx++)
	{
	    FlatView::Appearance& app = vwrs_[vidx]->appearance();
	    for (int auxidx=app.annot_.auxdata_.size()-1; auxidx>=0; auxidx--)
	    {
		if ( app.annot_.auxdata_[auxidx]==auxset[midx] )
		    app.annot_.auxdata_ -= auxset[midx];
	    }
	}
	delete auxset.remove(midx);
    }
}
