/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.4 2009-04-22 13:37:11 cvsbruno Exp $";

#include "uiwelltieview.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "flatposdata.h"
#include "iostrm.h"
#include "unitofmeasure.h"
#include "posinfo.h"
#include "position.h"
#include "posvecdataset.h"
#include "seisioobjinfo.h"
#include "survinfo.h"

#include "welldata.h"
#include "welld2tmodel.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltiesetup.h"
#include "welltiegeocalculator.h"
#include "welltrack.h"

#include "uiflatviewer.h"
#include "uiwelltiecontrolview.h"

uiWellTieView::uiWellTieView( uiParent* p, DataPointSet& dps, 
			      ObjectSet< Array1DImpl<float> >& dispdata, 
			      const Well::Data& d, const WellTieSetup& wts,
			      const Attrib::DescSet& ads )
	: dps_(dps)
	, dispdata_(dispdata)  
	, wd_(d)  
	, wtsetup_(wts)	
	, ads_(ads)	  
{
    if ( !wd_.d2TModel() ) return;
    createViewers( (uiGroup*)p );
} 


uiWellTieView::~uiWellTieView()
{
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
	vwrs_.remove(vwridx);
}


void uiWellTieView::setUpTimeAxis()
{
    timeintv_.start = dispdata_[0]->get(0);
    timeintv_.step =  SI().zStep();
    timeintv_.stop =  dispdata_[0]->get( dispdata_[0]->info().getSize(0)-1 );
}


void uiWellTieView::fullRedraw( uiGroup* vwrgrp )
{
    drawWellMarkers();
    drawVelLog();
    drawDensLog();
    drawSynthetics();
    drawSeismic();
    drawCShot();
}


void uiWellTieView::createViewers( uiGroup* vwrgrp )
{
    for (int vwridx=0; vwridx<4; vwridx++)
    {
	vwrs_ += new uiFlatViewer( vwrgrp, true);
	if ( vwridx>0 )
	    vwrs_[vwridx]->attach( rightOf, vwrs_[vwridx-1] );
    }

    initFlatViewer( wtsetup_.vellognm_, 0, 200, 550, false, Color::Black() );
    initFlatViewer( wtsetup_.denlognm_, 1, 200, 550, false, Color::Black() );
    initFlatViewer( "Synthetics", 2, 200, 550, true, Color::Black() );
    initFlatViewer( "Seismics", 3, 200, 550, true, Color::Black() );
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
    if ( wd_.checkShotModel() && wtsetup_.iscscorr_ ) 
	createVarDataPack( wtsetup_.corrvellognm_, 0, 1, 1 );
    else
	createVarDataPack( wtsetup_.vellognm_, 0, 1, 2 );
}


void uiWellTieView::drawDensLog()
{
    createVarDataPack( wtsetup_.denlognm_, 1, 1, 3 );
}


void uiWellTieView::drawSynthetics()
{
    BufferString synlbl = "Synthetics";
    createVarDataPack( synlbl, 2, 4, 6 );
}


void uiWellTieView::drawSeismic()
{
    const Attrib::Desc* ad = ads_.getDesc( wtsetup_.attrid_ );
    if ( !ad ) return;
    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    BufferStringSet bss;
    SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(0) ) );
    sii.getDefKeys( bss, true );
    const char* defkey = bss.get(0).buf();
    BufferString attrnm = ad->userRef();
    BufferString attr2cube = SeisIOObjInfo::defKey2DispName(defkey,attrnm);

    createVarDataPack( attr2cube, 3, 4, 7 );
}


void uiWellTieView::createVarDataPack( const char* varname, int vwrnr, 
					int nrtraces, int colidx )
{
    nrtraces = 1;
    float maxval, minval = 0;
    maxval = dispdata_[colidx]->get(0); minval = maxval;

    const int varsz = dispdata_[colidx]->info().getSize(0);
    Array2DImpl<float>*  arr2d = new Array2DImpl<float>( nrtraces, varsz );

    for ( int idz=0; idz< varsz; idz++)
    {
	float dataval =  dispdata_[colidx]->get(idz);
	if ( maxval < dataval && !mIsUdf(dataval) )
	    maxval = dataval;
	if ( minval > dataval && !mIsUdf(dataval) )
	    minval = dataval;
	for ( int idx=0; idx<nrtraces; idx++)
	    arr2d->set( idx, idz, dataval );
    }
    FlatView::Appearance& app = vwrs_[vwrnr]->appearance();
    app.ddpars_.wva_.overlap_ = nrtraces > 1 ? 3: maxval-minval-1;
    const float shift =  minval + ( maxval - minval )/2;
    StepInterval<double> xrange( shift, shift, maxval-minval);
    StepInterval<double> zrange( timeintv_.start, timeintv_.stop,
				 timeintv_.step );
    vwrs_[vwrnr]->removePack(0);
    FlatDataPack* dp = new FlatDataPack( "", arr2d );
    DPM(DataPackMgr::FlatID()).add( dp );
    if ( nrtraces < 2 )
	dp->posData().setRange(true, xrange);
    dp->posData().setRange(false, zrange);
    dp->setName( varname );
    vwrs_[vwrnr]->setPack( true, dp->id(), false, true );
    const UnitOfMeasure* uom = 0;
    const char* units =  ""; //uom ? uom->symbol() : "";
    app.annot_.x1_.name_ =  units;
    app.annot_.x2_.name_ = "TWT (ms)";

}


void uiWellTieView::drawMarker( FlatView::Annotation::AuxData* auxdata,
       				const int vwridx, const float zpos, Color col)
{
    FlatView::Appearance& app = vwrs_[vwridx]->appearance();
    app.annot_.auxdata_ +=  auxdata;
    auxdata->namepos_ = 0;
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.type_  = LineStyle::Solid;
   
    if ( col == Color::NoColor() || col.rgb() == 16777215 )
	col = Color::LightGrey();

    auxdata->poly_.erase();
    auxdata->poly_ += FlatView::Point(
	    vwrs_[vwridx]->boundingBox().left(), zpos );
    auxdata->poly_ += FlatView::Point(
	    vwrs_[vwridx]->boundingBox().right(), zpos );
}


void uiWellTieView::drawWellMarkers()
{
    bool ismarkergrdline = true;
  
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    int vwrsz = ismarkergrdline ? vwrs_.size() : 1;
    for ( int midx=0; midx<wd_.markers().size(); midx++ )
    {
	const Well::Marker* marker = wd_.markers()[midx];
	if ( !marker  ) continue;
	
	float zpos = d2tm->getTime( marker->dah() ); 
	
	FlatView::Annotation::AuxData* auxdata = new FlatView::Annotation::AuxData( 0 );
    
	for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	{
	    FlatView::Annotation::AuxData* a = 
			new FlatView::Annotation::AuxData(*auxdata);
	    drawMarker( a, vwridx, zpos, marker->color() );
	}
	delete auxdata;
    }
    for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}	


void uiWellTieView::drawUserPicks( const UserPicks* userpicks )
{
    if ( !userpicks  ) return;
    
    const int vwridx = userpicks->vieweridx_;
    if ( vwridx<0 || vwridx>vwrs_.size() )return;
    
  
    FlatView::Annotation::AuxData* auxdata = 0;
    mTryAlloc( auxdata, FlatView::Annotation::AuxData( 0 ) );
    
    const float zpos = userpicks->zpos_[userpicks->zpos_.size()-1];

    drawMarker( auxdata, vwridx, zpos, userpicks->color_ );
    userpickauxdatas_ += auxdata;

    vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}	 


void uiWellTieView::drawCShot()
{
    const Well::D2TModel* cs = wd_.checkShotModel();
    if ( !cs  ) return;
    WellTieGeoCalculator geocalc (wtsetup_,wd_);

    TypeSet<float> csvals, cstolog, dpt;
    for ( int idx=0; idx< cs->size(); idx++ )
    {
	dpt     += cs->dah(idx);
	csvals +=  cs->value(idx);
    }
    geocalc.TWT2Vel( csvals, dpt, cstolog, true );

    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    FlatView::Appearance& app = vwrs_[0]->appearance();
    
    for ( int idx=0; idx<app.annot_.auxdata_.size(); idx++)    
	delete ( app.annot_.auxdata_.remove(idx) );

    FlatView::Annotation::AuxData* auxdata = 0;
    mTryAlloc( auxdata, FlatView::Annotation::AuxData( 0 ) );
    app.annot_.auxdata_ +=  auxdata;
   
    for ( int midx=0; midx< cs->size(); midx++ )
    {
	auxdata->namepos_ = 0;
	auxdata->name_ = "checkshot";
	auxdata->linestyle_.color_ = Color::DgbColor();
	auxdata->linestyle_.type_  = LineStyle::Solid;
	float zpos = d2tm->getTime( cs->dah(midx) ); 
	auxdata->poly_ += FlatView::Point( cstolog[midx], zpos );
	auxdata->poly_ += FlatView::Point(  cstolog[midx], zpos );
    }

    vwrs_[0]->handleChange( FlatView::Viewer::Annot );
}


