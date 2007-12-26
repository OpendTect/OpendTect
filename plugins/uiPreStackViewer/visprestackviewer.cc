/*+
_______________________________________________________________________________

 COPYRIGHT:	(C) dGB Beheer B.V.
 AUTHOR:	Yuancheng Liu
 DAT:		May 2007
 RCS:           $Id: visprestackviewer.cc,v 1.8 2007-12-26 17:34:36 cvskris Exp $
_______________________________________________________________________________

 -*/

#include "visprestackviewer.h"

#include "iopar.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visfaceset.h"
#include "visflatviewer.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( PreStackView::PreStackViewer );

#define mDefaultWidth ((SI().inlDistance() + SI().crlDistance() ) * 100)

namespace PreStackView
{

PreStackViewer::PreStackViewer()
    : VisualObjectImpl( true )
    , draggerrect_( visBase::FaceSet::create() )
    , pickstyle_( visBase::PickStyle::create() )
    , planedragger_( visBase::DepthTabPlaneDragger::create() )	
    , flatviewer_( visBase::FlatViewer::create() )
    , bid_(-1,-1)
    , mid_( 0 )		
    , section_( 0 )
    , factor_( 1 )
    , width_( mDefaultWidth )
    , offsetrange_( 0, mDefaultWidth )
    , zrg_( SI().zRange(true) )
    , positiveside_( true )
    , autowidth_( true )
{
    setMaterial( 0 );
    planedragger_->ref();
    planedragger_->removeScaleTabs();
    planedragger_->motion.notify( mCB( this, PreStackViewer, draggerMotion ) );
    planedragger_->finished.notify( mCB( this, PreStackViewer, finishedCB ) );
    addChild( planedragger_->getInventorNode() );
    
    draggerrect_->ref();
    draggerrect_->removeSwitch();
    draggerrect_->setVertexOrdering( 
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    draggerrect_->setShapeType( 
	    visBase::VertexShape::cUnknownShapeType() );
    draggerrect_->getCoordinates()->addPos( Coord3( -1,-1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( 1,-1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( 1,1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( -1,1,0 ) );
    draggerrect_->setCoordIndex( 0, 0 );
    draggerrect_->setCoordIndex( 1, 1 );
    draggerrect_->setCoordIndex( 2, 2 );
    draggerrect_->setCoordIndex( 3, 3 );
    draggerrect_->setCoordIndex( 4, -1 );

    draggermaterial_ = visBase::Material::create();
    draggermaterial_->ref();
    draggerrect_->setMaterial( draggermaterial_ );
    draggermaterial_->setTransparency( 0.5 ); 

    planedragger_->setOwnShape( draggerrect_->getInventorNode() );
    
    pickstyle_->ref();
    addChild( pickstyle_->getInventorNode() );
    pickstyle_->setStyle( visBase::PickStyle::Unpickable );

    flatviewer_->ref();
    flatviewer_->setSelectable( false );
    flatviewer_->removeSwitch();
    flatviewer_->appearance().setGeoDefaults( true );

    flatviewer_->dataChange.notify( mCB( this,PreStackViewer,dataChangedCB ) );
    addChild( flatviewer_->getInventorNode() );
}


PreStackViewer::~PreStackViewer()
{
    planedragger_->motion.remove( mCB( this, PreStackViewer, draggerMotion ) );
    planedragger_->finished.remove( mCB( this, PreStackViewer, finishedCB ) );
    planedragger_->unRef();

    pickstyle_->unRef();
    draggerrect_->unRef();
    draggermaterial_->unRef();

    flatviewer_->dataChange.remove( mCB( this, PreStackViewer, dataChangedCB ));
    flatviewer_->unRef();
    
    if ( section_ )
    {
    	section_->getMovementNotifier()->remove( 
    		mCB( this, PreStackViewer, sectionMovedCB ) );
    	section_->unRef();
    }
}


void PreStackViewer::allowShading( bool yn )
{ flatviewer_->allowShading( yn ); }


void PreStackViewer::setColor( Color nc )
{ getMaterial()->setColor(nc); }


void  PreStackViewer::setMultiID( const MultiID& mid )
{ mid_ = mid; }
 

bool PreStackViewer::setPosition( const BinID& nb )
{
    if ( nb.inl==-1 || nb.crl==-1 )
    	turnOn(false);
    else
    {
	const bool haddata = flatviewer_->pack( false );
	bid_ = nb;
	PreStack::Gather* gather = new PreStack::Gather;
	if ( !gather->readFrom( mid_, bid_ ) )
	{
	    delete gather;
	    if ( haddata )
		 flatviewer_->setPack( false, DataPack::cNoID, false );
	    else
	    {
		dataChangedCB( 0 );
		return false;
	    }
	}
	else
	{
	    DPM(DataPackMgr::FlatID).add( gather );
	    flatviewer_->setPack( false, gather->id(), false, !haddata );
	}
	
	turnOn( true );
    }

    return true;
}


void PreStackViewer::displaysAutoWidth( bool yn )
{
    if ( autowidth_ == yn )
	return;

    autowidth_ = yn;
    dataChangedCB( 0 );
}


void PreStackViewer::displaysOnPositiveSide( bool yn )
{
    if ( positiveside_ == yn )
	return;

    positiveside_ = yn;
    dataChangedCB( 0 );
}


void PreStackViewer::setFactor( float scale )
{
    if (  factor_ == scale )
	return;

    factor_ = scale;
    dataChangedCB( 0 );
}


void PreStackViewer::setWidth( float width )
{
    if ( width_ == width )
	return;

    width_ = width;
    dataChangedCB( 0 );
}


void PreStackViewer::dataChangedCB( CallBacker* )
{
    if ( !section_ || factor_<0 || width_<0 )
	return;

    const bool offsetalonginl =
	section_->getOrientation()==visSurvey::PlaneDataDisplay::Crossline;

    const Coord direction = offsetalonginl
	? Coord( 0, positiveside_ ? 1 : -1 )
	: Coord( positiveside_ ? 1 : -1, 0 );

    const float offsetscale = offsetalonginl
	? SI().inlDistance()
	: SI().crlDistance();

    if ( offsetscale<=0 )
       return;	

    const FlatDataPack* fdp = flatviewer_->pack( false );
    if ( fdp )
    {
	offsetrange_.setFrom( fdp->posData().range( true ) );
	zrg_.setFrom( fdp->posData().range( false ) );
    }

    const Coord startpos( bid_.inl, bid_.crl );
    const Coord stoppos = autowidth_
	? startpos + direction*offsetrange_.width()*factor_ / offsetscale
	: startpos + direction*width_ / offsetscale;

    if ( autowidth_ )
	width_ = offsetrange_.width()*factor_;
    else
	factor_ = width_/offsetrange_.width();

    const Coord3 c00( startpos, zrg_.start );
    const Coord3 c01( stoppos, zrg_.start ); 
    const Coord3 c11( stoppos, zrg_.stop );
    const Coord3 c10( startpos, zrg_.stop );


    flatviewer_->setPosition( c00, c01, c10, c11 );
    planedragger_->setDim(
	section_->getOrientation()==visSurvey::PlaneDataDisplay::Inline ? 1:0 );

    const Coord3 width( fabs(stoppos.x-startpos.x),
	    	        fabs(stoppos.y-startpos.y), zrg_.width(true) );

    planedragger_->setSize( width );

    const Coord3 center( ( startpos+stoppos )/2, ( zrg_.start+zrg_.stop )/2 );
    planedragger_->setCenter( center );

    const Interval<float> xlim = Interval<float>( SI().inlRange( true ).start, 
	    					  SI().inlRange( true ).stop );
    const Interval<float> ylim = Interval<float>( SI().crlRange( true ).start,
	  					  SI().crlRange( true ).stop ); 

    planedragger_->setSpaceLimits( xlim, ylim, SI().zRange( true ) );    
    draggermaterial_->setTransparency( 1 ); 
}


const BinID& PreStackViewer::getPosition() const 
{ return bid_; }


const visSurvey::PlaneDataDisplay* PreStackViewer::getSectionDisplay() const
{ return section_;}


void PreStackViewer::setDisplayTransformation( visBase::Transformation* nt )
{ 
    flatviewer_->setDisplayTransformation( nt ); 
    planedragger_->setDisplayTransformation( nt );
}


void PreStackViewer::setSectionDisplay( visSurvey::PlaneDataDisplay* pdd )
{
    if ( section_ )
    {
	if ( section_->getMovementNotifier() )
	    section_->getMovementNotifier()->remove(
		    mCB( this, PreStackViewer, sectionMovedCB ) );
	section_->unRef();
    }

    section_ = pdd;
    if ( !section_ )
	return;

    section_->ref();
    dataChangedCB( 0 );

    if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Timeslice )
	return;
    
    if ( section_->getMovementNotifier() )
	section_->getMovementNotifier()->notify(
		mCB( this, PreStackViewer, sectionMovedCB ) );
}


void  PreStackViewer::sectionMovedCB( CallBacker* )
{
    BinID newpos = bid_;

    if ( !section_ )
	return;
    else
    {
    	if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
	    newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
    	else if ( section_->getOrientation() == 
		visSurvey::PlaneDataDisplay::Crossline )
	    newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    	else
	    return;
    }

    if ( !setPosition(newpos) )
	return;
}    


void  PreStackViewer::otherObjectsMoved( const ObjectSet<const SurveyObject>&
					 , int whichobj )
{
    if ( !section_ )
	return;

    if ( whichobj == -1 )
    {
	turnOn( section_->isShown() );
	return; 
    }

    if ( section_->id() != whichobj )
	return;
    
    turnOn( section_->isShown() );
}


void PreStackViewer::draggerMotion( CallBacker* )
{
    if ( !section_ )
	return;
    
    const int newinl = SI().inlRange( true ).snap( planedragger_->center().x );
    const int newcrl = SI().inlRange( true ).snap( planedragger_->center().y );

    const visSurvey::PlaneDataDisplay::Orientation orientation =
	    section_->getOrientation();

    bool showplane = false;
    if ( orientation==visSurvey::PlaneDataDisplay::Inline && newcrl!=bid_.crl )
        showplane = true;
    else if ( orientation==visSurvey::PlaneDataDisplay::Crossline &&
	      newinl!=bid_.inl )
	showplane = true;

    draggermaterial_->setTransparency( showplane ? 0.5 : 1 );
}


void  PreStackViewer::finishedCB( CallBacker* )
{ 
    if ( !section_ )
	return;

    int inl = SI().inlRange( true ).snap( planedragger_->center().x );
    int crl = SI().inlRange( true ).snap( planedragger_->center().y );
    BinID newpos = BinID( inl, crl );
    
    if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
	newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
    else if (section_->getOrientation() == 
	    visSurvey::PlaneDataDisplay::Crossline )
	newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    else
	return;

    if ( !setPosition(newpos) )
	return;
}


void PreStackViewer::getMousePosInfo( const visBase::EventInfo&,
				      const Coord3& pos, 
				      BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = "";
    if ( !flatviewer_ || !section_ ) 
	return;
    const FlatDataPack* fdp = flatviewer_->pack(false);
    if ( !fdp ) return;

    const FlatPosData& posdata = fdp->posData();
    const BinID bid = SI().transform( pos );
    const float distance = sqrt(bid_.sqDistTo( bid ));
    float offset = mUdf(float);
    
    if ( SI().inlDistance()==0 || SI().crlDistance()==0 || width_==0 )
	return;

    float start = posdata.range( true ).start;
    float cal = posdata.width(true)*distance/width_;
    if ( section_->getOrientation()==visSurvey::PlaneDataDisplay::Inline )
	offset = cal*SI().inlDistance()+start;
    else
	offset= cal*SI().crlDistance()+start;
   
    int offsetsample;
    float traceoffset;
    if ( posdata.isIrregular() )
    {
	float mindist;
	for ( int idx=0; idx<posdata.nrPts(true); idx++ )
	{
	    const float dist = fabs( posdata.position(true,idx)-offset );
	    if ( !idx || dist<mindist )
	    {
		offsetsample = idx;
		mindist = dist;
		traceoffset = posdata.position(true,idx);
	    }
	}
    }
    else
    {
	const StepInterval<double>& rg = posdata.range( true );
	offsetsample = rg.nearestIndex( offset );
	traceoffset = rg.atIndex( offsetsample );
    }

    info = "Offset: ";
    info += traceoffset;

    const int zsample = posdata.range(false).nearestIndex( pos.z );
    val = fdp->data().get( offsetsample, zsample );
}


void PreStackViewer::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
   if ( !section_ )
	return;

    SurveyObject::fillSOPar( par );
    VisualObjectImpl::fillPar( par, saveids );
    saveids.addIfNew( section_->id() );
    
    par.set( sKeyBinID(), bid_ );
    par.set( sKeyMultiID(), mid_ );
    par.setYN( sKeyiAutoWidth(), autowidth_ );
    par.setYN( sKeySide(), positiveside_ );
    par.set( sKeySectionID(), section_->id() );
    
    if ( autowidth_ )
	par.set( sKeyFactor(), factor_ );
    else
	par.set( sKeyWidth(), width_ );
}


int PreStackViewer::usePar( const IOPar& par )
{
    int res =  VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    res = SurveyObject::useSOPar( par );
    if ( res!=1 ) return res;

    int sectionid;
    if ( !par.get(sKeySectionID(),sectionid) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObject( sectionid );
    if ( !dataobj )
	return 0;

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj );
    if ( !pdd )
	return -1;

    setSectionDisplay( pdd );
	    	     
    BinID bid;
    MultiID mid;
    if ( !par.get(sKeyBinID(),bid) || !par.get(sKeyMultiID(),mid) )
	return -1;

    setMultiID( mid );
    if ( !setPosition( bid ) )
	return -1;
   
    
    float factor, width;
    if ( par.get(sKeyFactor(), factor) )
	setFactor( factor );

    if ( par.get(sKeyWidth(), width) )
	setWidth( width );

    bool autowidth, side;
    if ( par.getYN(sKeyiAutoWidth(), autowidth) )
	 displaysAutoWidth( autowidth );

    if ( par.getYN(sKeySide(), side) )
	displaysOnPositiveSide( side );
	
    return 1;
}


}; //namespace
