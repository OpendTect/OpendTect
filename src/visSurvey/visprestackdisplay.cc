/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Yuancheng Liu
 DAT:		May 2007
_______________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "visprestackdisplay.h"

#include "flatposdata.h"
#include "ioman.h"
#include "iopar.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "prestackevents.h"
#include "prestackgather.h"
#include "prestackprocessor.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisread.h"
#include "seispsioprov.h"
#include "sorting.h"
#include "survinfo.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visfaceset.h"
#include "visflatviewer.h"
#include "visforegroundlifter.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "visseis2ddisplay.h"
#include <math.h>

mCreateFactoryEntry( visSurvey::PreStackDisplay );

#define mDefaultWidth ((SI().inlDistance() + SI().crlDistance() ) * 100)

static const char* sKeyMultiID()	{ return "Data ID"; }
static const char* sKeyTraceNr()	{ return "TraceNr"; }

namespace visSurvey
{

PreStackDisplay::PreStackDisplay()
    : VisualObjectImpl( true )
    , draggerrect_( visBase::FaceSet::create() )
    , pickstyle_( visBase::PickStyle::create() )
    , planedragger_( visBase::DepthTabPlaneDragger::create() )	
    , flatviewer_( visBase::FlatViewer::create() )
    , draggermoving( this )
    , draggerpos_( -1, -1 )					 
    , bid_( -1, -1 )
    , mid_( 0 )		
    , section_( 0 )
    , seis2d_( 0 )
    , factor_( 1 )
    , trcnr_( -1 )
    , basedirection_( mUdf(float), mUdf(float) )		 
    , seis2dpos_( mUdf(float), mUdf(float) )		 
    , width_( mDefaultWidth )
    , offsetrange_( 0, mDefaultWidth )
    , zrg_( SI().zRange(true) )
    , posside_( true )
    , autowidth_( true )
    , preprocmgr_( 0 )			
    , reader_( 0 )
    , ioobj_( 0 )
    , movefinished_(this)
    , lifter_( visBase::ForegroundLifter::create() )  
{
    setMaterial( 0 );
    
    flatviewer_->ref();
    flatviewer_->setSelectable( false );
    flatviewer_->removeSwitch();
    flatviewer_->appearance().setGeoDefaults( true );
    flatviewer_->getMaterial()->setDiffIntensity( 0.2 );
    flatviewer_->getMaterial()->setAmbience( 0.8 );
    flatviewer_->appearance().ddpars_.vd_.mappersetup_.symmidval_ = 0;
    flatviewer_->dataChange.notify( mCB(this,PreStackDisplay,dataChangedCB) );
    addChild( flatviewer_->getInventorNode() );
    
    lifter_->ref();
    lifter_->setLift(0.8);
    addChild( lifter_->getInventorNode() );
    
    planedragger_->ref();
    planedragger_->removeScaleTabs();
    planedragger_->motion.notify( mCB(this,PreStackDisplay,draggerMotion) );
    planedragger_->finished.notify( mCB(this,PreStackDisplay,finishedCB) );
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
    pickstyle_->setStyle( visBase::PickStyle::Shape );
}


PreStackDisplay::~PreStackDisplay()
{
    lifter_->unRef();

    pickstyle_->unRef();
    draggerrect_->unRef();
    draggermaterial_->unRef();

    flatviewer_->dataChange.remove( mCB(this,PreStackDisplay,dataChangedCB) );
    flatviewer_->unRef();
    
    if ( section_ )
    {
	if ( planedragger_ )
	{ 
	    planedragger_->motion.remove(
		mCB(this,PreStackDisplay,draggerMotion) );
	    planedragger_->finished.remove(
		mCB(this,PreStackDisplay,finishedCB) );
	    planedragger_->unRef();
	}

    	section_->getMovementNotifier()->remove( 
		mCB(this,PreStackDisplay,sectionMovedCB) );
    	section_->unRef();
    }
    
    if ( seis2d_ )
    {
	if ( seis2d_->getMovementNotifier() )
    	    seis2d_->getMovementNotifier()->remove( 
		    mCB(this,PreStackDisplay,seis2DMovedCB) );
	seis2d_->unRef();
    }

    delete reader_;
    delete ioobj_;
}


void PreStackDisplay::allowShading( bool yn )
{ flatviewer_->allowShading( yn ); }


BufferString PreStackDisplay::getObjectName() const
{ return ioobj_->name(); }


void PreStackDisplay::setMultiID( const MultiID& mid )
{ 
    mid_ = mid;
    delete ioobj_; ioobj_ = IOM().get( mid_ );
    delete reader_; reader_ = 0;
    if ( !ioobj_ )
	return;

    if ( section_ )
	reader_ = SPSIOPF().get3DReader( *ioobj_ );
    else if ( seis2d_ )
	reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->name() );

    if ( !reader_ )
	return;

    if ( seis2d_ && seis2d_->getMovementNotifier() )
	seis2d_->getMovementNotifier()->notify(
		mCB(this,PreStackDisplay,seis2DMovedCB) );
    
    if ( section_ && section_->getMovementNotifier() )
	section_->getMovementNotifier()->notify(
		mCB(this,PreStackDisplay,sectionMovedCB) );
}


bool PreStackDisplay::setPreProcessor( PreStack::ProcessManager* mgr )
{
    preprocmgr_ = mgr;
    return updateData();
}


DataPack::ID PreStackDisplay::preProcess()
{
    if ( !ioobj_ || !reader_ )
	return -1;

    if ( !preprocmgr_ || !preprocmgr_->nrProcessors() || !preprocmgr_->reset() )
	return -1;

    if ( !preprocmgr_->prepareWork() )
	return -1;

    const BinID stepout = preprocmgr_->getInputStepout();

    BinID relbid;
    for ( relbid.inl=-stepout.inl; relbid.inl<=stepout.inl; relbid.inl++ )
    {
	for ( relbid.crl=-stepout.crl; relbid.crl<=stepout.crl; relbid.crl++ )
	{
	    if ( !preprocmgr_->wantsInput(relbid) )
		continue;
	 
	    const BinID inputbid = bid_ + 
		relbid*BinID(SI().inlStep(),SI().crlStep());
	    PreStack::Gather* gather = new PreStack::Gather;
	    if ( !gather->readFrom(*ioobj_,*reader_,inputbid) )
	    {
		delete gather;
		continue;
	    }

	    DPM( DataPackMgr::FlatID() ).addAndObtain( gather );
	    preprocmgr_->setInput( relbid, gather->id() );
	    DPM( DataPackMgr::FlatID() ).release( gather );
	}
    }
   
    if ( !preprocmgr_->process() )
	return -1;
   
    return preprocmgr_->getOutput();    
}


bool PreStackDisplay::setPosition( const BinID& nb )
{
    if ( bid_==nb )
	return true;

    bid_ = nb;

    PtrMan<PreStack::Gather> gather = new PreStack::Gather;
    if ( !ioobj_ || !reader_ || !gather->readFrom( *ioobj_, *reader_, nb ) )
    {
	static bool shown3d = false;
	static bool resetpos = true;
	if ( !shown3d )
	{
//	    resetpos = uiMSG().askContinue(
//		    "There is no data at the selected location.\n"
//		    "Do you want to find a nearby location to continue?" );
	    shown3d = true;
	}

	bool hasdata = false;
	if ( resetpos )
	{
	    BinID nearbid = getNearBinID( nb );
	    if ( nearbid.inl==-1 || nearbid.crl==-1 )
		;
//		uiMSG().warning("No gather data at the whole section.");
	    else
	    {
		bid_ = nearbid;
		hasdata = true;
	    }
        }
	else
	{
//	    uiMSG().warning("No gather data at the picked location.");
	}
	
    	if ( !hasdata )
	{
	    flatviewer_->appearance().annot_.x1_.showgridlines_ = false;
	    flatviewer_->appearance().annot_.x2_.showgridlines_ = false;
	    flatviewer_->turnOnGridLines( false, false );
	}
    }

    draggerpos_ = bid_;
    draggermoving.trigger();
    dataChangedCB( 0 );
    return updateData();
}


bool PreStackDisplay::updateData()
{
    if ( (is3DSeis() && (bid_.inl==-1 || bid_.crl==-1)) || 
	 (!is3DSeis() && !seis2d_) || !ioobj_ || !reader_ )
    {
	turnOn(false);
	return true;
    }

    const bool haddata = flatviewer_->pack( false );
    PreStack::Gather* gather = new PreStack::Gather;

    if ( is3DSeis() )
    {
	DataPack::ID displayid = DataPack::cNoID();
	if ( preprocmgr_ && preprocmgr_->nrProcessors() )
	{
	    displayid = preProcess();
	    delete gather;
	}
	else
	{
	    if ( !gather->readFrom( *ioobj_, *reader_, bid_ ) )
		delete gather;
	    else
	    {
    		DPM(DataPackMgr::FlatID()).add( gather );
    		displayid = gather->id();
	    }
	}

	if ( displayid==DataPack::cNoID() )
	{
	    if ( haddata )
		flatviewer_->setPack( false, DataPack::cNoID(), false );
	    else
		dataChangedCB( 0 );

	    return false;
	}
	else
	    flatviewer_->setPack( false, displayid, false, !haddata );
    }
    else
    {
	if ( !gather->readFrom( *ioobj_, *reader_, BinID(0,trcnr_) ) )
	{
	    delete gather;
	    if ( haddata )
		flatviewer_->setPack( false, DataPack::cNoID(), false );
	    else
	    {
		dataChangedCB( 0 );
		return false;
	    }
	}
	else
	{
	    DPM(DataPackMgr::FlatID()).add( gather );
	    flatviewer_->setPack( false, gather->id(), false, !haddata );
	}
    }

    turnOn( true );
    return true;  
}


const StepInterval<int> PreStackDisplay::getTraceRange( const BinID& bid ) const
{
    if ( is3DSeis() )
    {
	mDynamicCastGet( SeisPS3DReader*, rdr3d, reader_ );
	if ( !rdr3d ) 
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const PosInfo::CubeData& posinfo = rdr3d->posData();
	if ( isOrientationInline() )
	{
	    const int inlidx = posinfo.indexOf( bid.inl );
	    if ( inlidx==-1 )
		return StepInterval<int>(mUdf(int),mUdf(int),1);

	    const int seg = posinfo[inlidx]->nearestSegment( bid.crl );
	    return posinfo[inlidx]->segments_[seg];
	}
	else
	{
	    StepInterval<int> res;
	    posinfo.getInlRange( res );
	    return res;
	}
    }
    else
    {
	mDynamicCastGet( SeisPS2DReader*, rdr2d, reader_ );
	if ( !seis2d_ || !rdr2d ) 
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const TypeSet<PosInfo::Line2DPos>& posnrs
	    = rdr2d->posData().positions();
	const int nrtraces = posnrs.size();
	if ( !nrtraces )
	     return StepInterval<int>(mUdf(int),mUdf(int),1);

	mAllocVarLenArr( int, trcnrs, nrtraces );

	for ( int idx=0; idx<nrtraces; idx++ )
	    trcnrs[idx] = posnrs[idx].nr_;

	quickSort( mVarLenArr(trcnrs), nrtraces );
	const int trstep = nrtraces>1 ? trcnrs[1]-trcnrs[0] : 0;
	return StepInterval<int>( trcnrs[0], trcnrs[nrtraces-1], trstep );
    }
}


BinID PreStackDisplay::getNearBinID( const BinID& bid ) const
{
    const StepInterval<int> tracerg = getTraceRange( bid );
    if ( tracerg.isUdf() )
	return BinID(-1,-1);
    
    BinID res = bid;
    if ( isOrientationInline() )
    {
	res.crl = bid.crl<tracerg.start ? tracerg.start : 
	    ( bid.crl>tracerg.stop ? tracerg.stop : tracerg.snap(bid.crl) );
    }
    else
    {
	res.inl = bid.inl<tracerg.start ? tracerg.start : 
	    ( bid.inl>tracerg.stop ? tracerg.stop : tracerg.snap(bid.inl) );
    }

    return res;
}


int PreStackDisplay::getNearTraceNr( int trcnr ) const
{
    mDynamicCastGet(SeisPS2DReader*, rdr2d, reader_ );
    if ( !rdr2d )
	return -1;

    const TypeSet<PosInfo::Line2DPos>& posnrs = rdr2d->posData().positions();
    if ( posnrs.isEmpty() )
	return -1;

    int mindist=-1, residx=mUdf(int);
    for ( int idx=0; idx<posnrs.size(); idx++ )
    {
	const int dist = abs( posnrs[idx].nr_ - trcnr );
	if ( mindist==-1 || mindist>dist )
	{
	    mindist = dist;
	    residx = idx;
	}
    }   

   return posnrs[residx].nr_; 
}


void PreStackDisplay::displaysAutoWidth( bool yn )
{
    if ( autowidth_ == yn )
	return;

    autowidth_ = yn;
    dataChangedCB( 0 );
}


void PreStackDisplay::displaysOnPositiveSide( bool yn )
{
    if ( posside_ == yn )
	return;

    posside_ = yn;
    dataChangedCB( 0 );
}


void PreStackDisplay::setFactor( float scale )
{
    if (  factor_ == scale )
	return;

    factor_ = scale;
    dataChangedCB( 0 );
}

void PreStackDisplay::setWidth( float width )
{
    if ( width_ == width )
	return;

    width_ = width;
    dataChangedCB( 0 );
}


void PreStackDisplay::dataChangedCB( CallBacker* )
{
    if ( (!section_ && !seis2d_) || factor_<0 || width_<0 )
	return;

    if ( section_ && ( bid_.inl<0 || bid_.crl<0 ) )
	return;
	
    const Coord direction = posside_ ? basedirection_ : -basedirection_;
    const double offsetscale = Coord( basedirection_.x*SI().inlDistance(),
	    			     basedirection_.y*SI().crlDistance()).abs();

    const FlatDataPack* fdp = flatviewer_->pack( false );
    if ( fdp )
    {
	offsetrange_.setFrom( fdp->posData().range( true ) );
	zrg_.setFrom( fdp->posData().range( false ) );
    }

    if ( !offsetrange_.width() )
      	offsetrange_.stop = mDefaultWidth;

    Coord startpos( bid_.inl, bid_.crl );
    if ( seis2d_ )
	startpos = seis2dpos_;

    const Coord stoppos = autowidth_
	? startpos + direction*offsetrange_.width()*factor_ / offsetscale
	: startpos + direction*width_ / offsetscale;

    if ( seis2d_ )
	seis2dstoppos_ = stoppos;

    if ( autowidth_ )
	width_ = offsetrange_.width()*factor_;
    else
	factor_ = width_/offsetrange_.width();

    const Coord3 c00( startpos, zrg_.start );
    const Coord3 c01( startpos, zrg_.stop );
    const Coord3 c11( stoppos, zrg_.stop );
    const Coord3 c10( stoppos, zrg_.start ); 

    flatviewer_->setPosition( c00, c01, c10, c11 );
    if ( section_ )
    {
	bool isinline = 
	    section_->getOrientation()==PlaneDataDisplay::Inline;
	planedragger_->setDim( isinline ? 1 : 0 );

	const float xwidth = 
	    isinline ? (float) fabs(stoppos.x-startpos.x) : SI().inlDistance();
	const float ywidth = 
	    isinline ?  SI().crlDistance() : (float) fabs(stoppos.y-startpos.y);
    
    	planedragger_->setSize( Coord3(xwidth,ywidth,zrg_.width(true)) );
	
    	const Coord3 center( (startpos+stoppos)/2, (zrg_.start+zrg_.stop)/2 );
    	planedragger_->setCenter( center );

        Interval<float> xlim( mCast( float, SI().inlRange( true ).start ),
			      mCast( float, SI().inlRange( true ).stop ) );
        Interval<float> ylim( mCast( float, SI().crlRange( true ).start ),
			      mCast( float, SI().crlRange( true ).stop ) );
#define mBigNumber 1e15
	
	if ( isinline )
	    xlim.widen( mBigNumber );
	else
	    ylim.widen( mBigNumber );

	planedragger_->setSpaceLimits( xlim, ylim, SI().zRange( true ) );    
    }
    
    draggermaterial_->setTransparency( 1 );
}


const BinID& PreStackDisplay::getPosition() const 
{ return bid_; }


bool PreStackDisplay::isOrientationInline() const
{
    if ( !section_ )
	return false;

    return section_->getOrientation() == PlaneDataDisplay::Inline; 
}


const visSurvey::PlaneDataDisplay* PreStackDisplay::getSectionDisplay() const
{ return section_;}


void PreStackDisplay::setDisplayTransformation(
					const visBase::Transformation* nt )
{ 
    flatviewer_->setDisplayTransformation( nt ); 
    if ( planedragger_ )
	planedragger_->setDisplayTransformation( nt );

    if ( seis2d_ )
	seis2d_->setDisplayTransformation( nt );
}


void PreStackDisplay::setSectionDisplay( PlaneDataDisplay* pdd )
{
    if ( section_ )
    {
	if ( section_->getMovementNotifier() )
	    section_->getMovementNotifier()->remove(
		    mCB( this, PreStackDisplay, sectionMovedCB ) );
	section_->unRef();
    }

    section_ = pdd;
    if ( !section_ ) return;
    section_->ref();

    if ( ioobj_ && !reader_ )
    	reader_ = SPSIOPF().get3DReader( *ioobj_ );

    const int ctid = pdd->getColTabID(0);
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject(ctid) : 0;
    mDynamicCastGet(visBase::VisColorTab*,vct,obj);
    if ( vct )
    {
	flatviewer_->appearance().ddpars_.vd_.ctab_ = vct->colorSeq().name();
	flatviewer_->handleChange( FlatView::Viewer::DisplayPars );
    }

    const bool offsetalonginl = 
	section_->getOrientation()==PlaneDataDisplay::Crossline;
    basedirection_ = offsetalonginl ? Coord( 0, 1  ) : Coord( 1, 0 );

    if ( section_->getOrientation() == PlaneDataDisplay::Zslice )
	return;
    
    if ( section_->getMovementNotifier() )
	section_->getMovementNotifier()->notify(
		mCB( this, PreStackDisplay, sectionMovedCB ) );
}


void PreStackDisplay::sectionMovedCB( CallBacker* )
{
    BinID newpos = bid_;

    if ( !section_ )
	return;
    else
    {
    	if ( section_->getOrientation() == PlaneDataDisplay::Inline )
	    newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
    	else if ( section_->getOrientation() == 
		PlaneDataDisplay::Crossline )
	    newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    	else
	    return;
    }

    if ( !setPosition(newpos) )
	return;
}    


const visSurvey::Seis2DDisplay* PreStackDisplay::getSeis2DDisplay() const
{ return seis2d_; }


DataPack::ID PreStackDisplay::getDataPackID(int) const
{
    return flatviewer_->packID( false );
}


bool PreStackDisplay::is3DSeis() const
{
    return section_;
}


void PreStackDisplay::setTraceNr( int trcnr )
{
    if ( trcnr_==trcnr ) 
	return;

    if ( !seis2d_ )
	trcnr_ = trcnr;
    else
    {
    	PtrMan<PreStack::Gather> gather = new PreStack::Gather;
    	if ( !ioobj_ || !reader_ ||
	     !gather->readFrom(*ioobj_,*reader_,BinID(0,trcnr)) )
    	{
    	    static bool show2d = false;
    	    static bool resettrace = true;
    	    if ( !show2d )
    	    {
//		resettrace = uiMSG().askContinue(
//			"There is no data at the selected location.\n"
//			"Do you want to find a nearby location to continue?" );
    		show2d = true;
    	    }
	    
    	    if ( resettrace )
	    {
		trcnr_ = getNearTraceNr( trcnr );
		if ( trcnr_==-1 )
		{
//		    uiMSG().warning("Can not read or no data at the section.");
		    trcnr_ = trcnr; //If no data, we still display the panel.
		}
	    }
    	}
	else 
	    trcnr_ = trcnr;
    }

    draggermoving.trigger();
    seis2DMovedCB( 0 );
    updateData();
    turnOn( true );
}


#define mResetSeis2DPlane() \
    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) ); \
    basedirection_ = SI().binID2Coord().transformBackNoSnap( \
	    seis2d_->getNormal( trcnr_ )) - orig; \
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap( \
	    seis2d_->getCoord(trcnr_) );


bool PreStackDisplay::setSeis2DDisplay( Seis2DDisplay* s2d, int trcnr )
{
    if ( !s2d ) return false;

    if ( planedragger_ )
    { 
     	planedragger_->motion.remove( mCB(this,PreStackDisplay,draggerMotion) );
    	planedragger_->finished.remove( mCB(this,PreStackDisplay,finishedCB) );
    	planedragger_->unRef();
    }

    pickstyle_->setStyle( visBase::PickStyle::Shape );
    if ( seis2d_ ) 
    {
	if ( seis2d_->getMovementNotifier() )
    	    seis2d_->getMovementNotifier()->remove( 
		    mCB(this,PreStackDisplay,seis2DMovedCB) );

	seis2d_->unRef();
    }

    seis2d_ = s2d;
    seis2d_->ref();
     if ( ioobj_ && !reader_ )
	 reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->name() );

    setTraceNr( trcnr );
    if ( trcnr_<0 ) return false;

    const int ctid = s2d->getColTabID(0);
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject(ctid) : 0;
    mDynamicCastGet(visBase::VisColorTab*,vct,obj);
    if ( vct )
    {
	flatviewer_->appearance().ddpars_.vd_.ctab_ = vct->colorSeq().name();
	flatviewer_->handleChange( FlatView::Viewer::DisplayPars );
    }

    mResetSeis2DPlane();

    if ( seis2d_->getMovementNotifier() )
	seis2d_->getMovementNotifier()->notify( 
    		    mCB(this,PreStackDisplay,seis2DMovedCB) );

    return updateData();
}


void PreStackDisplay::seis2DMovedCB( CallBacker* )
{
    if ( !seis2d_ || trcnr_<0 )
	return;
    
    mResetSeis2DPlane();
    dataChangedCB(0);
}    


const Coord PreStackDisplay::getBaseDirection() const 
{ return basedirection_; }


const char* PreStackDisplay::lineName() const
{
    if ( !seis2d_ )
	return 0;

    return seis2d_->name();
}


void PreStackDisplay::otherObjectsMoved( const ObjectSet<const SurveyObject>&
					 , int whichobj )
{
    if ( !section_ && ! seis2d_ )
	return;

    if ( whichobj == -1 )
    {
	if ( section_ )
    	    turnOn( section_->isShown() );

	if ( seis2d_ )
	    turnOn( seis2d_->isShown() );
	return; 
    }

    if ( (section_ && section_->id() != whichobj) ||
	 (seis2d_ && seis2d_->id() != whichobj) )
	return;
    
    if ( section_ )
	turnOn( section_->isShown() );

    if ( seis2d_ )
	turnOn( seis2d_->isShown() );
}


void PreStackDisplay::draggerMotion( CallBacker* )
{
    if ( !section_ )
	return;
    
    const int newinl = SI().inlRange( true ).snap( planedragger_->center().x );
    const int newcrl = SI().crlRange( true ).snap( planedragger_->center().y );

    const PlaneDataDisplay::Orientation orientation =
	    section_->getOrientation();
    bool showplane = false;
    if ( orientation==PlaneDataDisplay::Inline && newcrl!=bid_.crl )
        showplane = true;
    else if ( orientation==PlaneDataDisplay::Crossline && 
	      newinl!=bid_.inl )
	showplane = true;
    
    draggermaterial_->setTransparency( showplane ? 0.5f : 1 );
    
    draggerpos_ = BinID(newinl, newcrl);
    draggermoving.trigger();
}


void PreStackDisplay::finishedCB( CallBacker* )
{ 
    if ( !section_ )
	return;

    BinID newpos;
    if ( section_->getOrientation() == PlaneDataDisplay::Inline )
    {
	newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
	newpos.crl = SI().crlRange(true).snap( planedragger_->center().y );
    }
    else if ( section_->getOrientation() ==
	    PlaneDataDisplay::Crossline )
    {
	newpos.inl = SI().inlRange(true).snap( planedragger_->center().x );
	newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    }
    else
	return;

    setPosition(newpos);
}


void PreStackDisplay::getMousePosInfo( const visBase::EventInfo& ei,
				      Coord3& pos, 
				      BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = "";
    if ( !flatviewer_  ) 
	return;

    const FlatDataPack* fdp = flatviewer_->pack(false);
    if ( !fdp ) return;

    const FlatPosData& posdata = fdp->posData();

    double offset = mUdf(double);
    const StepInterval<double>& rg = posdata.range( true );

    if ( seis2d_ )
    {
	info += "   Tracenr: ";
	info += trcnr_;
	const double displaywidth = seis2dstoppos_.distTo(seis2dpos_);
	double curdist = SI().binID2Coord().transformBackNoSnap( pos ).distTo( seis2dpos_ );
	offset = rg.start + posdata.width(true)*curdist/displaywidth;
	pos = Coord3( seis2dpos_, pos.z );
    }
    else if ( section_ )
    {
	const BinID bid = SI().transform( pos );
	const float distance = Math::Sqrt((float)bid_.sqDistTo( bid ));
	
	if ( SI().inlDistance()==0 || SI().crlDistance()==0 || width_==0 )
	    return;

	const float cal = posdata.width(true)*distance/width_;
	if ( section_->getOrientation()==PlaneDataDisplay::Inline )
	    offset = cal*SI().inlDistance()+rg.start;
	else
	    offset= cal*SI().crlDistance()+rg.start;
       
	pos = Coord3( SI().transform( bid_ ), pos.z );
    }

    int offsetsample = 0;
    double traceoffset;
    if ( posdata.isIrregular() )
    {
	float mindist = 0;
	for ( int idx=0; idx<posdata.nrPts(true); idx++ )
	{
	    const float dist = (float) fabs( posdata.position(true,idx)-offset );
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
	offsetsample = rg.nearestIndex( offset );
	if ( offsetsample<0 )
	    offsetsample = 0;
	else if ( offsetsample>rg.nrSteps() )
	    offsetsample = rg.nrSteps();

	traceoffset = rg.atIndex( offsetsample );
    }

    info = "Offset: ";
    info += traceoffset;

    const int zsample = posdata.range(false).nearestIndex( pos.z );
    val = fdp->data().get( offsetsample, zsample );

}


void PreStackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    if ( !section_ && !seis2d_ )
	return;

    SurveyObject::fillSOPar( par, saveids );
    VisualObjectImpl::fillPar( par, saveids );
    if ( section_ )
    {
	saveids.addIfNew( section_->id() );
        par.set( sKeyParent(), section_->id() );
	par.set( sKey::Position(), bid_ );
    }

    if  ( seis2d_ )
    {
	saveids.addIfNew( seis2d_->id() );
    	par.set( sKeyParent(), seis2d_->id() );
	par.set( sKeyTraceNr(), trcnr_ );
	//Line name is kept with parent
    }
    
    par.set( sKeyMultiID(), mid_ );
    par.setYN( sKeyAutoWidth(), autowidth_ );
    par.setYN( sKeySide(), posside_ );

    if ( flatviewer_ )
	flatviewer_->appearance().ddpars_.fillPar( par );   

    if ( autowidth_ )
	par.set( sKeyFactor(), factor_ );
    else
	par.set( sKeyWidth(), width_ );
}


int PreStackDisplay::usePar( const IOPar& par )
{
   int res =  VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    res = SurveyObject::useSOPar( par );
    if ( res!=1 ) return res;

    int parentid = -1;
    if ( !par.get( sKeyParent(), parentid ) )
    {
    	if ( !par.get( "Seis2D ID", parentid ) )
	    if ( !par.get( "Section ID", parentid ) )
		return -1;
    }

    visBase::DataObject* parent = visBase::DM().getObject( parentid );
    if ( !parent )
	return 0;

    MultiID mid;
    if ( !par.get( sKeyMultiID(), mid ) ) 
    {
	if ( !par.get("PreStack MultiID",mid) ) 
	{
	    return -1;
	}
    }
    
    setMultiID( mid );

    mDynamicCastGet( PlaneDataDisplay*, pdd, parent );
    mDynamicCastGet( Seis2DDisplay*, s2d, parent );
    if ( !pdd && !s2d )
	return -1;
    
    if ( pdd )
    {	
    	setSectionDisplay( pdd );
	BinID bid;
	if ( !par.get( sKey::Position(), bid ) )
	{
	    if ( !par.get("PreStack BinID",bid) )
		return -1;
	}

	if ( !setPosition( bid ) )
	    return -1;
    }

    if ( s2d )
    {
	int tnr;
	if ( !par.get( sKeyTraceNr(), tnr ) )
	{
	    if ( !par.get( "Seis2D TraceNumber", tnr ) )
		return -1;
	}

	setSeis2DDisplay( s2d, tnr );
    }
       
    float factor, width;
    if ( par.get(sKeyFactor(), factor) )
	setFactor( factor );

    if ( par.get(sKeyWidth(), width) )
	setWidth( width );

    bool autowidth, side;
    if ( par.getYN(sKeyAutoWidth(), autowidth) ) 
	 displaysAutoWidth( autowidth );

    if ( par.getYN(sKeySide(), side) )
	displaysOnPositiveSide( side );
	
    if ( flatviewer_ )
    {
	flatviewer_->appearance().ddpars_.usePar( par );   
	flatviewer_->handleChange( FlatView::Viewer::DisplayPars );
    }

    return 1;
}


} // namespace visSurvey
