/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieview.cc,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $";

#include "uiwelltieview.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "flatposdata.h"
#include "iostrm.h"
#include "unitofmeasure.h"
#include "posinfo.h"
#include "position.h"
#include "posvecdataset.h"
#include "seisioobjinfo.h"
#include "sorting.h"
#include "survinfo.h"

#include "welldata.h"
#include "welld2tmodel.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltiesetup.h"
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
    //peSet<float> zvals;
    //rtDPSDataAlongZ(zvals);
    timeintv_.start = dispdata_[0]->get(0);
    timeintv_.step =  dispdata_[0]->get(0)-dispdata_[0]->get(1);
    timeintv_.stop =  dispdata_[0]->get( dispdata_[0]->info().getSize(0)-1 );
}


void uiWellTieView::fullRedraw( uiGroup* vwrgrp )
{
    //for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
//	vwrs_.remove(vwridx);
    
    drawVelLog();
    drawDensLog();
    drawSynthetics();
    drawSeismic();
    //drawCShot();
    drawWellMarkers();
}


void uiWellTieView::createViewers( uiGroup* vwrgrp )
{
    for (int vwridx=0; vwridx<5; vwridx++)
    {
	vwrs_ += new uiFlatViewer( vwrgrp, true);
	if ( vwridx>0 )
	    vwrs_[vwridx]->attach( rightOf, vwrs_[vwridx-1] );
    }

    initFlatViewer( "Markers", 0, 150, 550, true, Color::Black() );
    initFlatViewer( wtsetup_.vellognm_, 1, 200, 550, false, Color::Black() );
    initFlatViewer( wtsetup_.denlognm_, 2, 200, 550, false, Color::Black() );
    initFlatViewer( "Synthetics", 3, 200, 550, true, Color::Black() );
    initFlatViewer( "Seismics", 4, 200, 550, true, Color::Black() );
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
	createVarDataPack( wtsetup_.corrvellognm_, 1, 1, 1 );
    else
	createVarDataPack( wtsetup_.vellognm_, 1, 1, 2 );
}


void uiWellTieView::drawDensLog()
{
    createVarDataPack( wtsetup_.denlognm_, 2, 1, 3 );
}


void uiWellTieView::drawSynthetics()
{
    BufferString synlbl = "Synthetics";
    createVarDataPack( synlbl, 3, 4, 6 );
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

    createVarDataPack( attr2cube, 4, 4, 7 );
}


void uiWellTieView::createVarDataPack( const char* varname, int vwrnr, 
					int nrtraces, int colidx )
{
    nrtraces = 1;
    TypeSet<float> zvals;
    float maxval =0; float minval = 0;
    //DataPointSet::ColID dpscolidx = dps_.indexOf( varname );
   // if ( dpscolidx >= 0 )
    //{
//	dps_.bivSet().getColumn( dpscolidx+4, data, true ) ;
	maxval = dispdata_[colidx]->get(0); minval = maxval;
  //  }
   

    const int varsz = dispdata_[colidx]->info().getSize(0);
    Array2DImpl<float>*  arr2d = new Array2DImpl<float>( nrtraces, varsz );
    
    for ( int idz=0; idz< varsz; idz++)
    {
	if ( colidx < 0 ) return;
	if ( maxval <= dispdata_[colidx]->get(idz) 
		&& !mIsUdf(dispdata_[colidx]->get(idz)) )
	    maxval = dispdata_[colidx]->get(idz);
	if ( maxval >= dispdata_[colidx]->get(idz) 
		&& !mIsUdf(dispdata_[colidx]->get(idz)) )
	    minval = dispdata_[colidx]->get(idz);
	for ( int idx=0; idx<nrtraces; idx++)
	    arr2d->set( idx, idz, dispdata_[colidx]->get(idz) );
    }
    FlatView::Appearance& app = vwrs_[vwrnr]->appearance();
    app.ddpars_.wva_.overlap_ = nrtraces > 1 ? 4: maxval-minval-1;
    const float shift =  minval + ( maxval - minval )/2;
    StepInterval<double> xrange( shift, shift, maxval-minval);
    StepInterval<double> zrange( timeintv_.start, timeintv_.stop,
	    			 timeintv_.step );
    vwrs_[vwrnr]->removePack(0);
    FlatDataPack* dp = new FlatDataPack( "", arr2d );
    DPM(DataPackMgr::FlatID()).add( dp );
    if ( nrtraces < 2 && colidx >= 0 )
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
    auxdata->linestyle_.color_ = col;
    auxdata->linestyle_.type_  = LineStyle::Solid;

    auxdata->poly_.erase();

    auxdata ->poly_ += FlatView::Point(
	    vwrs_[vwridx]->boundingBox().left(), zpos );
    auxdata->poly_ += FlatView::Point(
	    vwrs_[vwridx]->boundingBox().right(), zpos );
}


void uiWellTieView::drawWellMarkers()
{
    bool ismarkergrdline = true;
    //createVarDataPack( "Well Markers", 0, 1, 0 );
  
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

    int vwrsz = ismarkergrdline ? vwrs_.size() : 1;

    for ( int midx=0; midx<wd_.markers().size(); midx++ )
    {
	const Well::Marker* marker = wd_.markers()[midx];
	if ( !marker  ) continue;
	
	float zpos = d2tm->getTime( marker->dah() ); 
	
	FlatView::Annotation::AuxData* auxdata =
	    new FlatView::Annotation::AuxData(marker->name());
    
	for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	    drawMarker( auxdata, vwridx, zpos, marker->color() );
    }

    for ( int vwridx=0; vwridx<vwrsz; vwridx++ ) 
	vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}	


void uiWellTieView::drawUserPicks( const UserPicks* userpicks )
{
    if ( !userpicks  ) return;
    
    const int vwridx = userpicks->vieweridx_;
    if ( vwridx<0 || vwridx>vwrs_.size() )return;
    
  
    FlatView::Annotation::AuxData* auxdata =
	new FlatView::Annotation::AuxData("");
    
    const float zpos = userpicks->zpos_[userpicks->zpos_.size()-1];

    drawMarker( auxdata, vwridx, zpos, userpicks->color_ );
    userpickauxdatas_ += auxdata;

    vwrs_[vwridx]->handleChange( FlatView::Viewer::Annot );
}	 


void uiWellTieView::drawCShot()
{
    const Well::D2TModel* cs = wd_.checkShotModel();
    if ( !cs  ) return;
    
    float velfactor = wtsetup_.factors_.velFactor();
   
    TypeSet<float> cstolog, dpt;
    dpt += cs->dah(0);
    cstolog += cs->value(0);
    for ( int idx=1; idx< cs->size(); idx++ )
    {
	dpt[idx] = cs->dah(idx);
	cstolog += (cs->value(idx) - cs->value(idx-1))*1000;
    }

    cstolog[0] = cs->value(0)/(velfactor*dpt[0]); 
    for ( int idx=1; idx<cs->size(); idx++)
	cstolog[idx] = cstolog[idx]/(2*( dpt[idx]-dpt[idx-1] )*velfactor);

    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return; 

//To Remove
cstolog[0] += 150;
cstolog[1] += 50;
cstolog[2] -= 50;
cstolog[3] += 50;
cstolog[4] -= 50;

    int dpscolidx = 1;
    if ( dpscolidx<0 || dpscolidx>vwrs_.size() )return;
 
    FlatView::Appearance& app = vwrs_[dpscolidx]->appearance();
    
    for ( int idx=0; idx<app.annot_.auxdata_.size(); idx++)    
	delete ( app.annot_.auxdata_.remove(idx) );

    FlatView::Annotation::AuxData* auxdata =
	new FlatView::Annotation::AuxData("");
	app.annot_.auxdata_ +=  auxdata;
   
    for ( int midx=0; midx< cs->size(); midx++ )
    {
	auxdata->namepos_ = 0;
	auxdata->name_ = "checkshot";
	auxdata->linestyle_.color_ = Color::DgbColor();
	auxdata->linestyle_.type_  = LineStyle::Solid;
	float zpos = d2tm->getTime( cs->dah(midx) ); 
	auxdata ->poly_ += FlatView::Point( cstolog[midx], zpos );
	auxdata->poly_ += FlatView::Point(  cstolog[midx], zpos );
    }

    vwrs_[dpscolidx]->handleChange( FlatView::Viewer::Annot );
}


//TODO to remove since no dps any more
void uiWellTieView::sortDPSDataAlongZ( TypeSet<float>& zvals )
{
    for ( int idx=0; idx<dps_.size(); idx++ )
	zvals += dps_.z(idx);

    int sz = zvals.size();
    if ( !sz )  return;
    
    int zidxs[sz];
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), zidxs, sz );

    TypeSet<float> data; 
    for ( int colidx=1; colidx<dps_.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	    data += dps_.getValues(idx)[colidx];
	for ( int idx=0; idx<sz; idx++ )
	    dps_.getValues(idx)[colidx] = data[zidxs[idx]];
	data.erase();
    }
}

