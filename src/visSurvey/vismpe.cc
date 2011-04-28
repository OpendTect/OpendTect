/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismpe.cc,v 1.107 2011-04-28 07:00:12 cvsbert Exp $";

#include "vismpe.h"

#include "visboxdragger.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdatagroup.h"
#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexture3.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "visvolorthoslice.h"
#include "visselman.h"
#include "vistexturechannels.h"
#include "vistexturechannel2voldata.h"

#include "arrayndsubsel.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"

#include "arrayndimpl.h"

// This must be defined to use a texture to display the tracking plane.
// In future: Comment it out to use OrthogonalSlice (under construction...).
#define USE_TEXTURE 

mCreateFactoryEntry( visSurvey::MPEDisplay );

namespace visSurvey {

MPEDisplay::MPEDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
#ifdef USE_TEXTURE
    , texture_(0)
    , rectangle_(visBase::FaceSet::create())
    , draggerrect_(visBase::DataObjectGroup::create())
    , dragger_(0)
#else
    , isinited_(0)
    , allowshading_(false)
    , datatransform_(0)
    , cacheid_(DataPack::cNoID())
    , volumecache_(0)
    , channels_(visBase::TextureChannels::create())
    , voltrans_(visBase::Transformation::create())
    , dim_(2)
#endif
    , engine_(MPE::engine())
    , sceneeventcatcher_(0)
    , as_(*new Attrib::SelSpec())
    , manipulated_(false)
    , movement( this )
//    , slicemoving(this)
    , boxDraggerStatusChange( this )
    , planeOrientationChange( this )
    , curtexturecs_(false)
    , curtextureas_(*new Attrib::SelSpec())
{
    boxdragger_->ref();
    boxdragger_->finished.notify( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->setBoxTransparency( 0.7 );
    boxdragger_->turnOn( false );
    addChild( boxdragger_->getInventorNode() );
    updateBoxSpace();

#ifdef USE_TEXTURE
    draggerrect_->setSeparate( true );
    draggerrect_->ref();

    rectangle_->setVertexOrdering(
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
//  rectangle_->setFaceType(1);
    rectangle_->setMaterial( visBase::Material::create() );
    rectangle_->getCoordinates()->addPos( Coord3(-1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(-1,1,0) );
    rectangle_->setCoordIndex( 0, 0 );
    rectangle_->setCoordIndex( 1, 1 );
    rectangle_->setCoordIndex( 2, 2 );
    rectangle_->setCoordIndex( 3, 3 );
    rectangle_->setCoordIndex( 4, -1 );
    rectangle_->setTextureCoords( visBase::TextureCoords::create() );
    rectangle_->getTextureCoords()->addCoord( Coord3(0,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,1,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(0,1,0) );
    rectangle_->setTextureCoordIndex( 0, 0 );
    rectangle_->setTextureCoordIndex( 1, 1 );
    rectangle_->setTextureCoordIndex( 2, 2 );
    rectangle_->setTextureCoordIndex( 3, 3 );
    rectangle_->setTextureCoordIndex( 4, -1 );
    rectangle_->getMaterial()->setColor( Color::White() );
    draggerrect_->addObject( rectangle_ );

    visBase::IndexedPolyLine* polyline = visBase::IndexedPolyLine::create();
    polyline->setCoordinates( rectangle_->getCoordinates() );
    polyline->setCoordIndex( 0, 0 );
    polyline->setCoordIndex( 1, 1 );
    polyline->setCoordIndex( 2, 2 );
    polyline->setCoordIndex( 3, 3 );
    polyline->setCoordIndex( 4, 0 );
    polyline->setCoordIndex( 5, -1 );
    draggerrect_->addObject( polyline );

    setDragger( visBase::DepthTabPlaneDragger::create() );
    engine_.activevolumechange.notify( mCB(this,MPEDisplay,updateBoxPosition) );
    setDraggerCenter( true );
#else
    voltrans_->ref();
    addChild( voltrans_->getInventorNode() );
    voltrans_->setRotation( Coord3(0,1,0), M_PI_2 );

    channels_->ref();  // will be added in getInventorNode
    channels_->setChannels2RGBA( visBase::TextureChannel2VolData::create() );
    
    visBase::DM().getObject( channels_->getInventorNode() );
    
    CubeSampling cs(false); CubeSampling sics = SI().sampling(true);
    cs.hrg.start.inl = (5*sics.hrg.start.inl+3*sics.hrg.stop.inl)/8;
    cs.hrg.start.crl = (5*sics.hrg.start.crl+3*sics.hrg.stop.crl)/8;
    cs.hrg.stop.inl = (3*sics.hrg.start.inl+5*sics.hrg.stop.inl)/8;
    cs.hrg.stop.crl = (3*sics.hrg.start.crl+5*sics.hrg.stop.crl)/8;
    cs.zrg.start = ( 5*sics.zrg.start + 3*sics.zrg.stop ) / 8;
    cs.zrg.stop = ( 3*sics.zrg.start + 5*sics.zrg.stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    setCubeSampling( cs );
    
    engine_.activevolumechange.notify( mCB(this,MPEDisplay,updateBoxPosition) );
#endif

    updateBoxPosition(0);

    turnOn( true );
}


MPEDisplay::~MPEDisplay()
{
    engine_.activevolumechange.remove( mCB(this,MPEDisplay,updateBoxPosition) );

    setSceneEventCatcher( 0 );
#ifdef USE_TEXTURE
    setDragger(0);

    draggerrect_->unRef();
#else
    DPM( DataPackMgr::CubeID() ).release( cacheid_ );
    if ( volumecache_ )
	DPM( DataPackMgr::CubeID() ).release( volumecache_ );

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    voltrans_->unRef();
    channels_->unRef();

    setZAxisTransform( 0, 0 );
#endif

    boxdragger_->finished.remove( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->unRef();

    delete &as_;
    delete &curtextureas_;
}


void MPEDisplay::setColTabMapperSetup( int attrib,
				       const ColTab::MapperSetup& ms,
				       TaskRunner* tr )
{
#ifdef USE_TEXTURE
    if ( !texture_ ) return;
    visBase::VisColorTab& vt = texture_->getColorTab();

    const bool autoscalechange =
		ms.type_!=vt.colorMapper().setup_.type_ &&
		ms.type_!=ColTab::MapperSetup::Fixed;
    vt.colorMapper().setup_ = ms;
    if ( autoscalechange )
    {
	vt.autoscalechange.trigger();
	vt.colorMapper().setup_.triggerAutoscaleChange();
    }
    else
    {
	vt.rangechange.trigger();
	vt.autoscalechange.trigger();
	vt.colorMapper().setup_.triggerRangeChange();
    }
#else
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    channels_->setColTabMapperSetup( attrib, ms );
    channels_->reMapData( attrib, 0 );
#endif
}


void MPEDisplay::setColTabSequence( int attrib, const ColTab::Sequence& seq,
				    TaskRunner* tr )
{
#ifdef USE_TEXTURE
    if ( !texture_ ) return;

    visBase::VisColorTab& vt = texture_->getColorTab();
    vt.colorSeq().colors() = seq;
    vt.colorSeq().colorsChanged();
#else
    if ( attrib>=0 && attrib<nrAttribs() )	
	if ( channels_->getChannels2RGBA() )
	{
	    channels_->getChannels2RGBA()->setSequence( attrib, seq );
//	    channels_->reMapData( attrib, 0 );  // to do: check if necessary
	}
#endif
}


const ColTab::MapperSetup* MPEDisplay::getColTabMapperSetup( int attrib, 
	int version ) const
{ 
#ifdef USE_TEXTURE
    return texture_ ? &texture_->getColorTab().colorMapper().setup_ : 0; 
#else
    if ( attrib<0 || attrib>=nrAttribs() )
	return 0;

    if ( mIsUdf(version) || version<0
	    		 || version >= channels_->nrVersions(attrib) )
	version = channels_->currentVersion( attrib );

    return &channels_->getColTabMapperSetup( attrib, version );
#endif
}


const ColTab::Sequence* MPEDisplay::getColTabSequence( int attrib ) const
{ 
#ifdef USE_TEXTURE
    return texture_ ? &texture_->getColorTab().colorSeq().colors() : 0; 
#else
    return ( attrib>=0 && attrib<nrAttribs() && channels_->getChannels2RGBA() )
	? channels_->getChannels2RGBA()->getSequence( attrib ) : 0;
#endif
}


bool MPEDisplay::canSetColTabSequence() const
{ 
#ifdef USE_TEXTURE
    return true;
#else
    return ( channels_->getChannels2RGBA() ) ? 
	channels_->getChannels2RGBA()->canSetSequence() : false;
#endif
}


void MPEDisplay::setDragger( visBase::DepthTabPlaneDragger* dr )
{
#ifdef USE_TEXTURE
    if ( dragger_ )
    {
	dragger_->changed.remove( mCB(this,MPEDisplay,rectangleMovedCB) );
	dragger_->started.remove( mCB(this,MPEDisplay,rectangleStartCB) );
	dragger_->finished.remove( mCB(this,MPEDisplay,rectangleStopCB) );
	VisualObjectImpl::removeChild( dragger_->getInventorNode() );
	dragger_->unRef();
    }

    dragger_ = dr;
    if ( !dragger_ ) return;
        
    dragger_->ref();
    addChild( dragger_->getInventorNode() );
    dragger_->setOwnShape( draggerrect_->getInventorNode() );
    dragger_->setDim(0);
    dragger_->changed.notify( mCB(this,MPEDisplay,rectangleMovedCB) );
    dragger_->started.notify( mCB(this,MPEDisplay,rectangleStartCB) );
    dragger_->finished.notify( mCB(this,MPEDisplay,rectangleStopCB) );
#endif
}


CubeSampling MPEDisplay::getCubeSampling( int attrib ) const
{
#ifdef USE_TEXTURE
    return getBoxPosition();
#else
    return getCubeSampling( true, false, attrib );
#endif
}


CubeSampling MPEDisplay::getBoxPosition() const
{
    Coord3 center = boxdragger_->center();
    Coord3 width = boxdragger_->width();

    CubeSampling cube;
    cube.hrg.start = BinID( mNINT(center.x-width.x/2),
			    mNINT(center.y-width.y/2) );
    cube.hrg.stop = BinID( mNINT(center.x+width.x/2),
			   mNINT(center.y+width.y/2) );
    cube.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    cube.zrg.start = center.z - width.z / 2;
    cube.zrg.stop = center.z + width.z / 2;
    cube.zrg.step = SI().zStep();
    cube.hrg.snapToSurvey();
    SI().snapZ( cube.zrg.start, 0 );
    SI().snapZ( cube.zrg.stop, 0 );
    return cube;
}


bool MPEDisplay::getPlanePosition( CubeSampling& planebox ) const
{
#ifdef USE_TEXTURE
    const visBase::DepthTabPlaneDragger* drg = dragger_;
    const int dim = dragger_->getDim();
#else
    if ( ( !slices_.size() ) || ( !slices_[dim_] ) )
	return false;
    const visBase::DepthTabPlaneDragger* drg = slices_[dim_]->getDragger();
    const int dim = dim_;
#endif

    Coord3 center = drg->center();
    Coord3 size = drg->size();

#ifndef USE_TEXTURE
    if ( voltrans_ )
    {
	// seems to work even without this... check if required
	center = voltrans_->transform( center );
	Coord3 scale = voltrans_->getScale();
	size[0] *= scale[0];
	size[1] *= scale[1];
	size[2] *= scale[2];
    }
#endif

    if ( !dim )
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x);
	planebox.hrg.stop.inl = planebox.hrg.start.inl;

	planebox.hrg.start.crl = SI().crlRange(true).snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange(true).snap(center.y+size.y/2);

	planebox.zrg.start = SI().zRange(true).snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange(true).snap(center.z+size.z/2);
    }
    else if ( dim==1 )
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange(true).snap(center.x+size.x/2);

	planebox.hrg.stop.crl = SI().crlRange(true).snap(center.y);
	planebox.hrg.start.crl = planebox.hrg.stop.crl;

	planebox.zrg.start = SI().zRange(true).snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange(true).snap(center.z+size.z/2);
    }
    else 
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange(true).snap(center.x+size.x/2);

	planebox.hrg.start.crl = SI().crlRange(true).snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange(true).snap(center.y+size.y/2);

	planebox.zrg.stop = SI().zRange(true).snap(center.z);
	planebox.zrg.start = planebox.zrg.stop;
    }

    planebox.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    planebox.zrg.step = SI().zRange(true).step;

    return true;
}


void MPEDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    // Does checking for as_ = as give update problems? See comment of 
    // version 1.50 of vismultiattribsurvobj
    if ( attrib ) 
//    if ( attrib || as_ == as ) 
	return;
    as_ = as;

#ifndef USE_TEXTURE
    // from emptyCache of planedatadisplay
    DPM(DataPackMgr::CubeID()).release( volumecache_ );
    volumecache_ = 0;

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
    //////////////////////////////////////

    const char* usrref = as.userRef();
    BufferStringSet* attrnms = new BufferStringSet();
    attrnms->add( usrref );
    userrefs_.replace( attrib, attrnms );

    if ( ( !usrref || !*usrref ) && channels_->getChannels2RGBA() )
	channels_->getChannels2RGBA()->setEnabled( attrib, false );
    // check if last argument should be false or true
#endif
}


const Attrib::SelSpec* MPEDisplay::getSelSpec( int attrib ) const
{
    return attrib ? 0 : &as_;
}


const char* MPEDisplay::getSelSpecUserRef() const
{
    if ( as_.id()==Attrib::SelSpec::cNoAttrib() )
	return sKey::None;
    else if ( as_.id()==Attrib::SelSpec::cAttribNotSel() )
	return 0;

    return as_.userRef();
}


NotifierAccess* MPEDisplay::getMovementNotifier() 
{
    return &movement; 
/*#ifdef USE_TEXTURE
    return &movement; 
#else
    return &slicemoving;
#endif*/
}


NotifierAccess* MPEDisplay::getManipulationNotifier() 
{
#ifdef USE_TEXTURE
    return 0; 
#else
//    return &slicemoving;
    return &movement;
#endif
}


void MPEDisplay::updateTexture()
{
#ifdef USE_TEXTURE
    const CubeSampling displaycs = engine_.activeVolume();
    if ( curtextureas_==as_ && curtexturecs_==displaycs )
    {
	texture_->turnOn( true );
	return;
    }

    RefMan<const Attrib::DataCubes> attrdata = engine_.getAttribCache( as_ ) ?
	engine_.getAttribCache( as_ )->get3DData() : 0;
    if ( !attrdata )
    {
	if ( texture_ ) texture_->turnOn( false );
	return;
    }

    if ( !texture_ )
	setTexture( visBase::Texture3::create() );

    const Array3D<float>& data( attrdata->getCube(0) );

    if ( displaycs != attrdata->cubeSampling() )
    {
	const CubeSampling attrcs = attrdata->cubeSampling();
	if ( !attrcs.includes( displaycs ) )
	{
	    texture_->turnOn( false );
	    return;
	}

	const StepInterval<int> inlrg( attrcs.hrg.inlRange() );
	const StepInterval<int> crlrg( attrcs.hrg.crlRange() );
	const Interval<int> dispinlrg( inlrg.getIndex(displaycs.hrg.start.inl),
				       inlrg.getIndex(displaycs.hrg.stop.inl) );
	const Interval<int> dispcrlrg( crlrg.getIndex(displaycs.hrg.start.crl),
				       crlrg.getIndex(displaycs.hrg.stop.crl) );

	const StepInterval<float>& zrg( displaycs.zrg );

	const Interval<int> dispzrg( attrcs.zrg.nearestIndex( zrg.start ),
				     attrcs.zrg.nearestIndex( zrg.stop ) );

	const Array3DSubSelection<float> array( dispinlrg.start,dispcrlrg.start,
			  dispzrg.start, dispinlrg.width()+1,
			  dispcrlrg.width()+1,
			  dispzrg.width()+1,
			  const_cast< Array3D<float>& >(data) );

	if ( !array.isOK() )
	{
	    texture_->turnOn( false );
	    return;
	}

	texture_->setData( &array );
    }
    else
	texture_->setData( &data );

    curtextureas_ = as_;
    curtexturecs_ = displaycs;

    texture_->turnOn( true );
#endif    
}


void MPEDisplay::setTexture( visBase::Texture3* nt )
{
#ifdef USE_TEXTURE
    if ( texture_ )
    {
	int oldindex = draggerrect_->getFirstIdx( (const DataObject*)texture_ );
	if ( oldindex!=-1 )
	    draggerrect_->removeObject( oldindex );
    }

    texture_ = nt;
    if ( texture_ )
	draggerrect_->insertObject( 0, (DataObject*)texture_ );

    updateTextureCoords();
#endif
}


void MPEDisplay::moveMPEPlane( int nr )
{
#ifdef USE_TEXTURE
    visBase::DepthTabPlaneDragger* drg = dragger_;
    if ( !drg || !nr ) return;
    const int dim = dragger_->getDim();
#else
    if ( ( !slices_.size() ) || ( !slices_[dim_] ) )
	return;
    visBase::DepthTabPlaneDragger* drg = slices_[dim_]->getDragger();
    if ( !drg || !nr ) return;
    const int dim = dim_;
#endif

    Coord3 center = drg->center();
    Coord3 width = boxdragger_->width();

    Interval<float> sx, sy, sz;
    drg->getSpaceLimits( sx, sy, sz );
    
#ifndef USE_TEXTURE
    if ( voltrans_ )
    {
	center = voltrans_->transform( center );
	Coord3 spacelim( sx.start, sy.start, sz.start );
	spacelim = voltrans_->transform( spacelim );
	sx.start = spacelim.x;	sy.start = spacelim.y;	sz.start = spacelim.z;
	spacelim = voltrans_->transform( Coord3( sx.stop, sy.stop, sz.stop ) ); 
	sx.stop = spacelim.x;	sy.stop = spacelim.y;	sz.stop = spacelim.z;
    }
#endif

    center.x = 0.5 * ( SI().inlRange(true).snap( center.x - width.x/2 ) +
	    	       SI().inlRange(true).snap( center.x + width.x/2 ) );
    center.y = 0.5 * ( SI().crlRange(true).snap( center.y - width.y/2 ) +
	    	       SI().crlRange(true).snap( center.y + width.y/2 ) );
    center.z = 0.5 * ( SI().zRange(true).snap( center.z - width.z/2 ) +
		       SI().zRange(true).snap( center.z + width.z/2 ) );

    const int nrsteps = abs(nr);
    const float sign = nr > 0 ? 1.001 : -1.001;
    // sign is slightly to big to avoid that it does not trigger a track
    
    sx.widen( 0.5*SI().inlStep(), true );
    sy.widen( 0.5*SI().crlStep(), true );
    sz.widen( 0.5*SI().zStep(), true );
    // assure that border lines of survey are reachable in spite of foregoing
    
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	if ( !dim )
	    center.x += sign * SI().inlStep();
	else if ( dim==1 )
	    center.y += sign * SI().crlStep();
	else
	    center.z += sign * SI().zStep();

	if ( !sx.includes(center.x) || !sy.includes(center.y) || 
	     !sz.includes(center.z) )
	    return;
	
	Coord3 newcenter( center );
#ifdef USE_TEXTURE
	drg->setCenter( newcenter, false );
#else
        if ( voltrans_ )
	    newcenter = voltrans_->transformBack( center );
	slices_[dim_]->setCenter( newcenter, false );
#endif
    }
}


void MPEDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
#ifdef USE_TEXTURE
    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->eventhappened.remove(
					mCB(this,MPEDisplay,mouseClickCB) );
	sceneeventcatcher_->unRef();
    }

    sceneeventcatcher_ = nevc;

    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->ref();
	sceneeventcatcher_->eventhappened.notify(
	    mCB(this,MPEDisplay,mouseClickCB) );
    }
#else
    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->eventhappened.remove(
	    mCB(this,MPEDisplay,updateMouseCursorCB) );
	sceneeventcatcher_->unRef();
    }

    sceneeventcatcher_ = nevc;

    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->ref();
	sceneeventcatcher_->eventhappened.notify(
	    mCB(this,MPEDisplay,updateMouseCursorCB) );
    }
#endif
}


void MPEDisplay::updateMouseCursorCB( CallBacker* cb )
{
    mouseClickCB( cb );
    return;

#ifndef USE_TEXTURE
    char newstatus = 1; // 1=pan, 2=tabs
    if ( cb )
    {
	mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
	if ( eventinfo.pickedobjids.indexOf(boxdragger_->id())==-1 )
	    newstatus = 0;
	else
	{
	    //TODO determine if tabs
	}
    }

    if ( !isSelected() || !isOn() || isLocked() )
	newstatus = 0;

    if ( !newstatus ) mousecursor_.shape_ = MouseCursor::NotSet;
    else if ( newstatus==1 ) mousecursor_.shape_ = MouseCursor::SizeAll;
#endif
}


void MPEDisplay::boxDraggerFinishCB(CallBacker*)
{
#ifdef USE_TEXTURE
    const CubeSampling newcube = getBoxPosition();
    if ( newcube!=engine_.activeVolume() )
    {
//	if ( texture_ ) texture_->turnOn(false);
	manipulated_ = true;
    }
#else
    if ( scene_ && scene_->getZAxisTransform() )
        return;

    CubeSampling cs = getCubeSampling( true, true, 0 );
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    Interval<int> inlrg( cs.hrg.start.inl, cs.hrg.stop.inl );
    Interval<int> crlrg( cs.hrg.start.crl, cs.hrg.stop.crl );
    Interval<float> zrg( cs.zrg.start, cs.zrg.stop );
    SI().checkInlRange( inlrg, true );
    SI().checkCrlRange( crlrg, true );
    SI().checkZRange( zrg, true );
    if ( inlrg.start == inlrg.stop ||
	 crlrg.start == crlrg.stop ||
	 mIsEqual(zrg.start,zrg.stop,1e-8) )
    {
	resetManipulation();
	return;
    }
    else
    {
	cs.hrg.start.inl = inlrg.start; cs.hrg.stop.inl = inlrg.stop;
	cs.hrg.start.crl = crlrg.start; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = zrg.start; cs.zrg.stop = zrg.stop;
    }

    const Coord3 newwidth( cs.hrg.stop.inl - cs.hrg.start.inl,
			   cs.hrg.stop.crl - cs.hrg.start.crl,
			   cs.zrg.stop - cs.zrg.start );
    boxdragger_->setWidth( newwidth );
    const Coord3 newcenter( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			    (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			    (cs.zrg.stop + cs.zrg.start) / 2 );
    boxdragger_->setCenter( newcenter );

    manipulated_ = true;
#endif // to do: check ifdef
}


void MPEDisplay::showBoxDragger( bool yn )
{
    if ( yn==boxdragger_->isOn() )
	return;

    boxdragger_->turnOn( yn );
    boxDraggerStatusChange.trigger();
}


void MPEDisplay::updateSeedOnlyPropagation( bool yn )
{
    engine_.updateSeedOnlyPropagation( yn );
}


void MPEDisplay::updateMPEActiveVolume()
{
    if ( manipulated_ )
    {
	const CubeSampling newcube = getBoxPosition();
	engine_.setActiveVolume( newcube );
	manipulated_ = false;
    }
}


void MPEDisplay::removeSelectionInPolygon( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    engine_.removeSelectionInPolygon( selector, tr );
    manipulated_ = true;
}


bool MPEDisplay::isOn() const
{
    return visBase::VisualObjectImpl::isOn() &&
	( isBoxDraggerShown() || isDraggerShown() );
}


bool MPEDisplay::isBoxDraggerShown() const
{ return boxdragger_->isOn(); }


void MPEDisplay::setDraggerTransparency( float transparency )
{
#ifdef USE_TEXTURE
    rectangle_->getMaterial()->setTransparency( transparency );
#else
    // to do: check if this needs to be moved to setAttribTransparency
    mDynamicCastGet( visBase::TextureChannel2VolData*, cttc2vd,
		channels_->getChannels2RGBA() );
    /*if ( cttc2vd )
          cttc2vd->setTransparency( attrib, nt );*/
        // to do: implement in TextureChannel2VolData
#endif
}


float MPEDisplay::getDraggerTransparency() const
{
#ifdef USE_TEXTURE
    return rectangle_->getMaterial()->getTransparency();
#else
    // to do: check if this needs to be moved to getAttribTransparency
    mDynamicCastGet( visBase::TextureChannel2VolData*, cttc2vd,
	    channels_->getChannels2RGBA() );
    /*if ( cttc2vd )
	return cttc2vd->getTransparency( attrib );*/
    // to do: implement in TextureChannel2VolData
    return 0;
#endif
}


void MPEDisplay::showDragger( bool yn )
{
    if ( yn==isDraggerShown() )
	return;
    
#ifdef USE_TEXTURE
    if ( yn )
	updateTexture();
    dragger_->turnOn( yn );
#else
	// to do: check!
    if ( yn )
	updateSlice();

    if ( slices_.size() && slices_[dim_] )
        slices_[dim_]->turnOn( yn );
#endif

    movement.trigger();
    planeOrientationChange.trigger();
}


bool MPEDisplay::isDraggerShown() const
{ 
#ifdef USE_TEXTURE
    return dragger_->isOn(); 
#else
    if ( slices_.size() && slices_[dim_] )
        return slices_[dim_]->isOn();
    return false;
#endif
}


void MPEDisplay::rectangleMovedCB( CallBacker* )
{
#ifdef USE_TEXTURE
    if ( isSelected() ) return;

    while( true ) {
	MPE::TrackPlane newplane = engine_.trackPlane();
	CubeSampling& planebox = newplane.boundingBox();
	getPlanePosition( planebox );

	if ( planebox==engine_.trackPlane().boundingBox() )
	    return;

	updateTextureCoords();

	const CubeSampling& engineplane = engine_.trackPlane().boundingBox();
	const int dim = dragger_->getDim();
	if ( !dim && planebox.hrg.start.inl==engineplane.hrg.start.inl )
	    return;
	if ( dim==1 && planebox.hrg.start.crl==engineplane.hrg.start.crl )
	    return;
	if ( dim==2 && mIsEqual( planebox.zrg.start, engineplane.zrg.start, 
				 0.1*SI().zStep() ) )
	    return;

	if ( !dim )
	{
	    const bool inc = planebox.hrg.start.inl>engineplane.hrg.start.inl;
	    int& start = planebox.hrg.start.inl;
	    int& stop =  planebox.hrg.stop.inl;
	    const int step = SI().inlStep();
	    start = stop = engineplane.hrg.start.inl + ( inc ? step : -step );
	    newplane.setMotion( inc ? step : -step, 0, 0 );
	}
	else if ( dim==1 )
	{
	    const bool inc = planebox.hrg.start.crl>engineplane.hrg.start.crl;
	    int& start = planebox.hrg.start.crl;
	    int& stop =  planebox.hrg.stop.crl;
	    const int step = SI().crlStep();
	    start = stop = engineplane.hrg.start.crl + ( inc ? step : -step );
	    newplane.setMotion( 0, inc ? step : -step, 0 );
	}
	else 
	{
	    const bool inc = planebox.zrg.start>engineplane.zrg.start;
	    float& start = planebox.zrg.start;
	    float& stop =  planebox.zrg.stop;
	    const double step = SI().zStep();
	    start = stop = engineplane.zrg.start + ( inc ? step : -step );
	    newplane.setMotion( 0, 0, inc ? step : -step );
	}
	const MPE::TrackPlane::TrackMode trkmode = newplane.getTrackMode();
	engine_.setTrackPlane( newplane, trkmode==MPE::TrackPlane::Extend
				      || trkmode==MPE::TrackPlane::ReTrack
				      || trkmode==MPE::TrackPlane::Erase );
	movement.trigger();
	planeOrientationChange.trigger();
	}
#endif
}


void MPEDisplay::rectangleStartCB( CallBacker* )
{
#ifdef USE_TEXTURE
    Undo& undo = EM::EMM().undo();
    lasteventnr_ = undo.currentEventID();
#endif
}


void MPEDisplay::rectangleStopCB( CallBacker* )
{
#ifdef USE_TEXTURE
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent!=lasteventnr_ )
	undo.setUserInteractionEnd(currentevent);
#endif
}


void MPEDisplay::setPlaneOrientation( int orient )
{
#ifdef USE_TEXTURE
    dragger_->setDim( orient );
    
    if ( !isOn() ) return;

    updateTextureCoords();
#else
    if ( ( orient < 0 ) || ( orient > 2 ) )
	return;
    dim_ = orient;

    for ( int i = 0; i < 3; i++ )
        slices_[i]->turnOn( dim_ == i );
    
    updateRanges( true, true );
#endif
    movement.trigger();
}


const int MPEDisplay::getPlaneOrientation() const
{ 
#ifdef USE_TEXTURE
    return dragger_->getDim(); 
#else
    return dim_;
#endif
}


void MPEDisplay::mouseClickCB( CallBacker* cb )
{
//#ifdef USE_TEXTURE
    if ( sceneeventcatcher_->isHandled() || !isOn() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick )
	return;

    if ( OD::leftMouseButton(eventinfo.buttonstate_) &&
	 OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	 eventinfo.pickedobjids.indexOf(id())!=-1 )
    {
	if ( eventinfo.pressed )
	{
#ifdef USE_TEXTURE
 	    int dim = dragger_->getDim();
	    if ( ++dim>=3 )
		dim = 0;
	    dragger_->setDim( dim );
	    MPE::TrackPlane ntp = engine_.trackPlane();
	    getPlanePosition( ntp.boundingBox() );
	    engine_.setTrackPlane( ntp, false );
	    updateTextureCoords();
#else
	    if ( ++dim_>=3 )
		dim_ = 0;
	    for ( int i = 0; i < 3; i++ )
		slices_[i]->turnOn( dim_ == i );
	    MPE::TrackPlane ntp = engine_.trackPlane();
	    getPlanePosition( ntp.boundingBox() );
	    engine_.setTrackPlane( ntp, false );
	    updateRanges( true, true );
#endif
	    movement.trigger();
	    planeOrientationChange.trigger();
	}
	sceneeventcatcher_->setHandled();
    }
    else if ( OD::rightMouseButton( eventinfo.buttonstate_ ) &&
	      OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	      eventinfo.pickedobjids.indexOf(id())!=-1 && isDraggerShown() )
    {
	if ( eventinfo.pressed )
	{
	    const MPE::TrackPlane::TrackMode tm = 
					engine_.trackPlane().getTrackMode();
	    if ( tm==MPE::TrackPlane::Move )
		engine_.setTrackMode( MPE::TrackPlane::Extend );
	    else if ( tm==MPE::TrackPlane::Extend )
		engine_.setTrackMode( MPE::TrackPlane::ReTrack );
	    else if ( tm==MPE::TrackPlane::ReTrack )
		engine_.setTrackMode( MPE::TrackPlane::Erase );
	    else 
		engine_.setTrackMode( MPE::TrackPlane::Move );
	}
	sceneeventcatcher_->setHandled();
    }
    else if ( OD::leftMouseButton(eventinfo.buttonstate_) &&
	      !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	      isBoxDraggerShown() &&
	      eventinfo.pickedobjids.indexOf(boxdragger_->id())==-1 )
    {
	showBoxDragger( false );
	sceneeventcatcher_->setHandled();
    }
//#endif
}


void MPEDisplay::freezeBoxPosition( bool yn )
{
    if ( yn )
    {
    	engine_.activevolumechange.remove( 
				   mCB(this,MPEDisplay,updateBoxPosition) );
    }
    else
    {
    	engine_.activevolumechange.notifyIfNotNotified( 
				   mCB(this,MPEDisplay,updateBoxPosition) );
    }
}


void MPEDisplay::updateBoxPosition( CallBacker* )
{
#ifdef USE_TEXTURE
    NotifyStopper stop( dragger_->changed );
#endif

    CubeSampling cube = engine_.activeVolume();
    Coord3 newwidth( cube.hrg.stop.inl-cube.hrg.start.inl,
		     cube.hrg.stop.crl-cube.hrg.start.crl,
		     cube.zrg.stop-cube.zrg.start );

    // Workaround for deadlock in COIN's polar_decomp() or Math::Sqrt(), which
    // occasionally occurs in case the box has one side of zero length.
    if ( cube.hrg.nrInl()==1 )
	newwidth.x = 0.1 * cube.hrg.step.inl;
    if ( cube.hrg.nrCrl()==1 )
	newwidth.y = 0.1 * cube.hrg.step.crl;
    if ( cube.nrZ()==1 )
	newwidth.z = 0.1 * cube.zrg.step;

    boxdragger_->setWidth( newwidth );
#ifdef USE_TEXTURE
    dragger_->setSize( newwidth );
#endif

    const Coord3 newcenter( (cube.hrg.stop.inl+cube.hrg.start.inl)/2,
			    (cube.hrg.stop.crl+cube.hrg.start.crl)/2,
			    cube.zrg.center());

    boxdragger_->setCenter( newcenter );

#ifdef USE_TEXTURE
    dragger_->setSpaceLimits(
	    Interval<float>(cube.hrg.start.inl,cube.hrg.stop.inl),
	    Interval<float>(cube.hrg.start.crl,cube.hrg.stop.crl),
	    Interval<float>(cube.zrg.start,cube.zrg.stop) );

    setDraggerCenter( true );
    if ( isDraggerShown() )
	updateTexture();

    updateTextureCoords();
#else
    // check if all of the below is needed
    //turnOnSlice( false );
    channels_[0].turnOn( false );

    const Interval<float> xintv( cube.hrg.start.inl, cube.hrg.stop.inl );
    const Interval<float> yintv( cube.hrg.start.crl, cube.hrg.stop.crl );
    const Interval<float> zintv( cube.zrg.start, cube.zrg.stop );
    voltrans_->setTranslation(
	    Coord3(xintv.center(),yintv.center(),zintv.center()) );
    voltrans_->setRotation( Coord3( 0, 1, 0 ), M_PI_2 );
    voltrans_->setScale( Coord3(-zintv.width(),yintv.width(),xintv.width()) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setSpaceLimits( Interval<float>(-0.5,0.5),
		Interval<float>(-0.5,0.5),
		Interval<float>(-0.5,0.5) );

    if ( isDraggerShown() )
	updateSlice();
#endif
    
    movement.trigger();
    planeOrientationChange.trigger();
}


void MPEDisplay::updateBoxSpace()
{
    const HorSampling& hs = SI().sampling(true).hrg;
    const Interval<float> survinlrg( hs.start.inl, hs.stop.inl );
    const Interval<float> survcrlrg( hs.start.crl, hs.stop.crl );
    const Interval<float> survzrg( SI().zRange(true).start,
	    			   SI().zRange(true).stop );

    boxdragger_->setSpaceLimits( survinlrg, survcrlrg, survzrg );
}


void MPEDisplay::updateDraggerPosition( CallBacker* cb )
{
    setDraggerCenter( false );
}


void MPEDisplay::setDraggerCenter( bool alldims )
{
#ifdef USE_TEXTURE
    NotifyStopper stop( dragger_->changed );
    const CubeSampling& cs = engine_.trackPlane().boundingBox();
    if ( cs.hrg.start.inl==cs.hrg.stop.inl && dragger_->getDim()!=0 )
	dragger_->setDim(0);
    else if ( cs.hrg.start.crl==cs.hrg.stop.crl && dragger_->getDim()!=1 )
	dragger_->setDim(1);
    else if ( !cs.zrg.width() && dragger_->getDim()!=2 ) dragger_->setDim(2);

    const Coord3 newcenter((cs.hrg.stop.inl+cs.hrg.start.inl)/2,
			   (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
			   cs.zrg.center());
    if ( newcenter != dragger_->center() )
	dragger_->setCenter( newcenter, alldims );
#endif
}


#define mGetRelCrd(val,dim) \
		(val-boxcenter[dim]+boxwidth[dim]/2)/boxwidth[dim]

void MPEDisplay::updateTextureCoords()
{
#ifdef USE_TEXTURE
    if ( !dragger_ ) return;
    Coord3 boxcenter = boxdragger_->center();
    Coord3 boxwidth = boxdragger_->width();

    const Coord3 draggercenter = dragger_->center();
    const Coord3 draggerwidth = dragger_->size();
    const int dim = dragger_->getDim();

    const float relcoord = mGetRelCrd(draggercenter[dim],dim);
    const Interval<float> intv0( 
	    mGetRelCrd(draggercenter[0]-draggerwidth[0]/2,0),
	    mGetRelCrd(draggercenter[0]+draggerwidth[0]/2,0) );
    const Interval<float> intv1( 
	    mGetRelCrd(draggercenter[1]-draggerwidth[1]/2,1),
	    mGetRelCrd(draggercenter[1]+draggerwidth[1]/2,1) );
    const Interval<float> intv2( 
	    mGetRelCrd(draggercenter[2]-draggerwidth[2]/2,2),
	    mGetRelCrd(draggercenter[2]+draggerwidth[2]/2,2) );

    if ( !dim )
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(relcoord,intv1.start,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(relcoord,intv1.start,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(relcoord,intv1.stop,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 3,
				Coord3(relcoord,intv1.stop,intv2.start) );
    }
    else if ( dim==1 )
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(intv0.start,relcoord,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(intv0.stop,relcoord,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(intv0.stop,relcoord,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 3, 
				Coord3(intv0.start,relcoord,intv2.stop) );
    }
    else
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(intv0.start,intv1.start,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(intv0.stop,intv1.start,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(intv0.stop,intv1.stop,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 3, 
				Coord3(intv0.start,intv1.stop,relcoord) );
    }
#endif
}


float MPEDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    CubeSampling cs; 
    if ( !getPlanePosition(cs) )
	return mUdf(float);

    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl =
	binid.inl>=cs.hrg.start.inl && binid.inl<=cs.hrg.stop.inl
	     ? 0
	     : mMIN( abs(binid.inl-cs.hrg.start.inl),
		     abs( binid.inl-cs.hrg.stop.inl) );
    inlcrldist.crl =
        binid.crl>=cs.hrg.start.crl && binid.crl<=cs.hrg.stop.crl
             ? 0
	     : mMIN( abs(binid.crl-cs.hrg.start.crl),
		     abs( binid.crl-cs.hrg.stop.crl) );
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    zdiff = cs.zrg.includes(xytpos.z)
	     ? 0
	     : mMIN(xytpos.z-cs.zrg.start,xytpos.z-cs.zrg.stop) *
	       zfactor  * scene_->getZStretch();

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return Math::Sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}

    
float MPEDisplay::maxDist() const
{
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    float maxzdist = zfactor * scene_->getZStretch() * SI().zStep() / 2;
    return engine_.trackPlane().boundingBox().nrZ()==1 
					? maxzdist : SurveyObject::sDefMaxDist();
}


void MPEDisplay::getMousePosInfo( const visBase::EventInfo&, Coord3& pos,
				  BufferString& val, BufferString& info ) const
{
    val = "undef";
    info = "";

#ifdef USE_TEXTURE
    const BinID bid( SI().transform(pos) );
    RefMan<const Attrib::DataCubes> attrdata = engine_.getAttribCache( as_ ) ? 
	    engine_.getAttribCache( as_ )->get3DData() : 0;
    if ( !attrdata )
	return;

    const CubeSampling& datacs = attrdata->cubeSampling();
    if ( !datacs.hrg.includes(bid) || !datacs.zrg.includes(pos.z) )
	return;
    const float fval = attrdata->getCube(0).get( datacs.inlIdx(bid.inl),
	    					 datacs.crlIdx(bid.crl),
						 datacs.zIdx(pos.z) );
    if ( !mIsUdf(fval) )
	val = fval;

    const int dim = dragger_->getDim();
    CubeSampling planecs;
    getPlanePosition( planecs );

    if ( !dim )
    {
	info = "Inline: ";
	info += planecs.hrg.start.inl;
    }
    else if ( dim==1 )
    {
	info = "Crossline: ";
	info += planecs.hrg.start.crl;
    }
    else
    {
	info = SI().zIsTime() ? "Time: " : "Depth: ";
	const float z = planecs.zrg.start;
	info += SI().zIsTime() ? mNINT( z * 1000) : z;
    }
#else
    if ( !isBoxDraggerShown() )
        val = getValue( pos );
#endif
}


void MPEDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

#ifdef USE_TEXTURE
    if ( texture_ )
    {
	par.set( sKeyTexture(), texture_->id() );
	if ( saveids.indexOf(texture_->id())==-1 )
	    saveids += texture_->id();
    }
#else
    mDynamicCastGet( visBase::TextureChannel2VolData*, cttc2vd,
                     channels_ ? channels_->getChannels2RGBA() : 0 );
    if ( !cttc2vd )
    {
	par.set( sKeyTC2VolData(), channels_->getChannels2RGBA()->id() );
	saveids += channels_->getChannels2RGBA()->id();
    }
#endif

    as_.fillPar( par );
    par.set( sKeyTransparency(), getDraggerTransparency() );
    par.setYN( sKeyBoxShown(), isBoxDraggerShown() );
}


int MPEDisplay::usePar( const IOPar& par )
{
    const int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

#ifdef USE_TEXTURE
    int textureid;
    if ( par.get(sKeyTexture(),textureid) )
    {
	mDynamicCastGet( visBase::Texture3*, texture,
		         visBase::DM().getObject(textureid) );
	if ( texture ) setTexture( texture );
	else return 0;
    }
#else

    int tc2vdid;
    if ( par.get( sKeyTC2VolData(), tc2vdid ) )
    {
	RefMan<visBase::DataObject> dataobj =
	    visBase::DM().getObject( tc2vdid );
	if ( !dataobj )
	    return 0;

	mDynamicCastGet(visBase::TextureChannel2VolData*, tc2vd, dataobj.ptr());
	if ( tc2vd )
	    setChannels2VolData( tc2vd );
    }
#endif

    float transparency = 0.5;
    par.get( sKeyTransparency(), transparency );
    setDraggerTransparency( transparency );

    bool dispboxdragger = false;
    par.getYN( sKeyBoxShown(), dispboxdragger );

    if ( as_.usePar( par ) )
#ifdef USE_TEXTURE
    updateTexture();
#else
    //();
#endif
    turnOn( true );
    showBoxDragger( dispboxdragger );
    
    return 1;
}


visBase::OrthogonalSlice* MPEDisplay::getSlice( int index )
{
#ifndef USE_TEXTURE
    if ( ( index >= 0 ) && ( index < slices_.size() ) )
        return slices_[index];
#endif
    return 0;
}



void MPEDisplay::setCubeSampling( const CubeSampling& cs )
{
#ifndef USE_TEXTURE
    const Interval<float> xintv( cs.hrg.start.inl, cs.hrg.stop.inl );
    const Interval<float> yintv( cs.hrg.start.crl, cs.hrg.stop.crl );
    const Interval<float> zintv( cs.zrg.start, cs.zrg.stop );
    voltrans_->setTranslation(
	    Coord3(xintv.center(),yintv.center(),zintv.center()) );
    voltrans_->setRotation( Coord3( 0, 1, 0 ), M_PI_2 );
    voltrans_->setScale( Coord3(-zintv.width(),yintv.width(),xintv.width()) );
    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setSpaceLimits( Interval<float>(-0.5,0.5),
		Interval<float>(-0.5,0.5),
		Interval<float>(-0.5,0.5) );

//  if ( channels_[0] ) channels_[0].turnOn( false );  // check why

    resetManipulation();
#endif
}


void MPEDisplay::resetManipulation()
{
#ifndef USE_TEXTURE
    const Coord3 center = voltrans_->getTranslation();
    const Coord3 width = voltrans_->getScale();
    boxdragger_->setCenter( center );
    boxdragger_->setWidth( Coord3(width.z, width.y, -width.x) );
#endif
}


bool MPEDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tr )
{
#ifndef USE_TEXTURE
    // to do: check if needs to be copied from setDisplayDataPackIDs
    if ( attrib>0 ) return false;

    DataPackMgr& dpman = DPM( DataPackMgr::CubeID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    if ( !cdp )
    {
	dpman.release( dpid );
	return false;
    }

    const bool res = setDataVolume( attrib, cdp, tr );
    
    if ( volumecache_ )
	dpman.release( volumecache_ );
    volumecache_ = cdp;
    return true;
#else
    return false;
#endif
}


bool MPEDisplay::setDataVolume( int attrib, const Attrib::CubeDataPack* cdp, 
				   TaskRunner* tr )
{
#ifdef USE_TEXTURE
    return false;
#else
    if ( !cdp )
	return false;

    DataPack::ID attridpid = cdp->id();
    DPM( DataPackMgr::CubeID() ).obtain( attridpid );

    //transform data if necessary.
    const char* zdomain = getSelSpec( attrib )->zDomainKey();
    const bool alreadytransformed = zdomain && *zdomain;
	
    if ( !alreadytransformed && datatransform_ )
    {
	// to do: check this stuff
	ZAxisTransformer* datatransformer;
	mTryAlloc( datatransformer,ZAxisTransformer(*datatransform_,true));
	datatransformer->setInterpolate( textureInterpolationEnabled() );
	//datatransformer->setInterpolate( true );
	datatransformer->setInput( cdp->cube().getCube(0), cdp->sampling() );
	datatransformer->setOutputRange( getCubeSampling(true,true,0) );
		
	if ( (tr && tr->execute(*datatransformer)) ||
             !datatransformer->execute() )
	{
	    pErrMsg( "Transform failed" );
	    return false;
	}

	CubeDataPack cdpnew( cdp->categoryStr( false ), 
	// check false for categoryStr
	datatransformer->getOutput( true ) );
	DPM( DataPackMgr::CubeID() ).addAndObtain( &cdpnew );
	DPM( DataPackMgr::CubeID() ).release( attridpid );
	attridpid = cdpnew.id();
    }

    updateFromDataPackID( attrib, attridpid, tr );
    DPM( DataPackMgr::CubeID() ).release( attridpid );

//    setCubeSampling( getCubeSampling(true,true,0) );
   
    return true;
#endif
}


void MPEDisplay::updateFromDataPackID( int attrib, const DataPack::ID newdpid,
				       TaskRunner* tr )
{
#ifndef USE_TEXTURE
    DPM(DataPackMgr::CubeID()).release( cacheid_ );

    cacheid_ = newdpid;
    DPM(DataPackMgr::CubeID()).obtain( cacheid_ );

    updateFromCacheID( attrib, tr );
#endif
}


void MPEDisplay::updateFromCacheID( int attrib, TaskRunner* tr )
{
    channels_->setNrVersions( attrib, 1 );  // to do: check if necessary

    const DataPack* datapack = DPM(DataPackMgr::CubeID()).obtain( cacheid_ );
    mDynamicCastGet( const Attrib::CubeDataPack*, cdp, datapack );
    if ( !cdp )
    {
	channels_->turnOn( false );
	DPM(DataPackMgr::CubeID()).release( cacheid_ );
	return;
    }

    const Array3D<float>* dparr = 
	&const_cast <Attrib::CubeDataPack*>(cdp)->data();

    const float* arr = dparr->getData();
    OD::PtrPolicy cp = OD::UsePtr;

    int sz0 = dparr->info().getSize(0);
    int sz1 = dparr->info().getSize(1);
    int sz2 = dparr->info().getSize(2);

    if ( !arr )
    {
	const od_int64 totalsz = sz0*sz1*sz2;
	mDeclareAndTryAlloc( float*, tmparr, float[totalsz] );

	if ( !tmparr )
	{
	    DPM(DataPackMgr::CubeID()).release( cacheid_ );		
	    return;
	}
	else
	{
	    dparr->getAll( tmparr );

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}
    }

    channels_[0].setSize( sz2, sz1, sz0 );
    channels_[0].setUnMappedData( attrib, 0, arr, cp, tr );
    channels_->reMapData( 0, 0 );

    //rectangle_->setOriginalTextureSize( sz0, sz1 );

    channels_[0].turnOn( true );
    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( sz2, sz1, sz0 ); 
}


void MPEDisplay::updateSlice()
{
}


const Attrib::DataCubes* MPEDisplay::getCacheVolume( int attrib ) const
{ 
#ifndef USE_TEXTURE
    return ( volumecache_ && !attrib ) ? &volumecache_->cube() : 0;
#else
    return 0;
#endif
}


DataPack::ID MPEDisplay::getDataPackID( int attrib ) const
{ 
#ifndef USE_TEXTURE
    return attrib==0 ? cacheid_ : DataPack::cNoID();
#else
    return 0;
#endif

}


CubeSampling MPEDisplay::getCubeSampling( bool manippos, bool displayspace,
	  			          int attrib ) const
{
#ifndef USE_TEXTURE
    CubeSampling res;
    if ( manippos )
    {
	Coord3 center_ = boxdragger_->center();
	Coord3 width_ = boxdragger_->width();

	res.hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
		mNINT( center_.y - width_.y / 2 ) );

	res.hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
		mNINT( center_.y + width_.y / 2 ) );

	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = center_.z - width_.z / 2;
	res.zrg.stop = center_.z + width_.z / 2;
    }
    else
    {
	const Coord3 transl = voltrans_->getTranslation();
	Coord3 scale = voltrans_->getScale();
	double dummy = scale.x; scale.x=scale.z; scale.z = dummy;

	res.hrg.start = BinID( mNINT(transl.x+scale.x/2),
		mNINT(transl.y+scale.y/2) );
	res.hrg.stop = BinID( mNINT(transl.x-scale.x/2),
		mNINT(transl.y-scale.y/2) );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = transl.z+scale.z/2;
	res.zrg.stop = transl.z-scale.z/2;
    }

    if ( alreadyTransformed(attrib) ) return res;

    if ( datatransform_ && !displayspace )
    {
	res.zrg.setFrom( datatransform_->getZInterval(true) );
	res.zrg.step = SI().zRange( true ).step;
    }

    return res;
#else
    return 0;
#endif
}


int MPEDisplay::addSlice( int dim, bool show )
{
#ifndef USE_TEXTURE
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->turnOn( show );
    slice->setMaterial(0);
    slice->setDim(dim);
    slice->motion.notify( mCB(this,MPEDisplay,sliceMoving) );
    slices_ += slice;

    slice->setName( dim==cTimeSlice() ? sKeyTime() : 
	    (dim==cCrossLine() ? sKeyCrossLine() : sKeyInline()) );

    addChild( slice->getInventorNode() );
    const CubeSampling cs = getCubeSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    if ( volumecache_ )
    {
	const Array3D<float>& arr = volumecache_->cube().getCube(0);
	slice->setVolumeDataSize( arr.info().getSize(2),
		arr.info().getSize(1), arr.info().getSize(0) );
    }

    return slice->id();
#else
    return 0;
#endif
}


float MPEDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
#ifndef USE_TEXTURE
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepos = slice->getPosition();
    slicepos *= -voltrans_->getScale()[dim];

    float pos;
    if ( dim == 2 )
    {
	slicepos += voltrans_->getTranslation()[0];
	pos = SI().inlRange(true).snap(slicepos);
    }
    else if ( dim == 1 )
    {
	slicepos += voltrans_->getTranslation()[1];
	pos = SI().crlRange(true).snap(slicepos);
    }
    else
    {
	slicepos += voltrans_->getTranslation()[2];
	pos = mNINT(slicepos*1000);
    }

    return pos;
#else
    return 0;
#endif
}


float MPEDisplay::getValue( const Coord3& pos_ ) const
{
#ifndef USE_TEXTURE
    if ( !volumecache_ ) return mUdf(float);
    const BinIDValue bidv( SI().transform(pos_), pos_.z );
    float val;
    if ( !volumecache_->cube().getValue(0,bidv,&val,false) )
        return mUdf(float);

    return val;
#else
    return 0;
#endif
}


SoNode* MPEDisplay::gtInvntrNode()
{
#ifndef USE_TEXTURE
    if ( !isinited_ )
    {
	isinited_ = true;
	//channels_[0].useShading( allowshading_ );

	const int voltransidx = childIndex( voltrans_->getInventorNode() );
	insertChild( voltransidx+1, channels_->getInventorNode() );
	
	channels_->turnOn( true );

	if ( !slices_.size() )
	{
	    addSlice( cInLine(), false );
	    addSlice( cCrossLine(), false );
	    addSlice( cTimeSlice(), false );
	}
    }
#endif

    return VisualObjectImpl::gtInvntrNode();
}


void MPEDisplay::allowShading( bool yn )
{
#ifndef USE_TEXTURE
    if ( channels_ && channels_->getChannels2RGBA() )
	channels_->getChannels2RGBA()->allowShading( yn );
#endif
}


void MPEDisplay::removeChild( int displayid )
{
#ifndef USE_TEXTURE
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->getInventorNode() );
	    slices_[idx]->motion.remove( mCB(this,MPEDisplay,sliceMoving) );
	    slices_[idx]->unRef();
	    slices_.remove(idx,false);
	    return;
	}
    }
#endif
}


void MPEDisplay::getChildren( TypeSet<int>&res ) const
{
#ifndef USE_TEXTURE
    res.erase();
    for ( int idx=0; idx<slices_.size(); idx++ )
	res += slices_[idx]->id();
#endif
}


bool MPEDisplay::isSelected() const
{
#ifdef USE_TEXTURE
    return VisualObjectImpl::isSelected();
#else
    return visBase::DM().selMan().selected().indexOf( id()) != -1;	
#endif
}


BufferString MPEDisplay::getManipulationString() const
{
    BufferString res;
    getObjectInfo( res );
    return res;
}


void MPEDisplay::getObjectInfo( BufferString& info ) const
{
    info = slicename_; info += ": "; info += sliceposition_;
}


void MPEDisplay::sliceMoving( CallBacker* cb )
{
#ifndef USE_TEXTURE
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice ) return;

    slicename_ = slice->name();
    sliceposition_ = slicePosition( slice );
//    movement/*slicemoving*/.trigger();

    if ( isSelected() ) return;
	
    while( true ) {
	MPE::TrackPlane newplane = engine_.trackPlane();
	CubeSampling& planebox = newplane.boundingBox();
	// get the position of the plane from the dragger
	getPlanePosition( planebox );

	// nothing to do if dragger is in sync with engine
	if ( planebox==engine_.trackPlane().boundingBox() )
	    return;

	// bring slice to the position specified by dragger
	// will be done automatically by the callback on dragger of slice?
	updateSlice();

	const CubeSampling& engineplane = engine_.trackPlane().boundingBox();
	const int dim = slice->getDragger()->getDim();
	if ( !dim && planebox.hrg.start.inl==engineplane.hrg.start.inl )
	    return;
	if ( dim==1 && planebox.hrg.start.crl==engineplane.hrg.start.crl )
	    return;
	if ( dim==2 && mIsEqual( planebox.zrg.start, engineplane.zrg.start, 
				 0.1*SI().zStep() ) )
	    return;

	if ( !dim )
	{
	    const bool inc = planebox.hrg.start.inl>engineplane.hrg.start.inl;
	    int& start = planebox.hrg.start.inl;
	    int& stop =  planebox.hrg.stop.inl;
	    const int step = SI().inlStep();
	    start = stop = engineplane.hrg.start.inl + ( inc ? step : -step );
	    newplane.setMotion( inc ? step : -step, 0, 0 );
	}
	else if ( dim==1 )
	{
	    const bool inc = planebox.hrg.start.crl>engineplane.hrg.start.crl;
	    int& start = planebox.hrg.start.crl;
	    int& stop =  planebox.hrg.stop.crl;
	    const int step = SI().crlStep();
	    start = stop = engineplane.hrg.start.crl + ( inc ? step : -step );
	    newplane.setMotion( 0, inc ? step : -step, 0 );
	}
	else 
	{
	    const bool inc = planebox.zrg.start>engineplane.zrg.start;
	    float& start = planebox.zrg.start;
	    float& stop =  planebox.zrg.stop;
	    const double step = SI().zStep();
	    start = stop = engineplane.zrg.start + ( inc ? step : -step );
	    newplane.setMotion( 0, 0, inc ? step : -step );
	}
	const MPE::TrackPlane::TrackMode trkmode = newplane.getTrackMode();
	engine_.setTrackPlane( newplane, trkmode==MPE::TrackPlane::Extend
				      || trkmode==MPE::TrackPlane::ReTrack
				      || trkmode==MPE::TrackPlane::Erase );
	movement.trigger();
	planeOrientationChange.trigger();
    }
#endif
}


void MPEDisplay::showManipulator( bool yn )
{
#ifndef USE_TEXTURE
    showBoxDragger( yn ); 
#endif
}


bool MPEDisplay::isManipulated() const
{
#ifdef USE_TEXTURE
    return false;
#else
    return getCubeSampling(true,true,0) != getCubeSampling(false,true,0);
#endif
}


bool MPEDisplay::canResetManipulation() const
{
#ifdef USE_TEXTURE
    return false;
#else
    return true;
#endif
}


void MPEDisplay::acceptManipulation()
{
#ifndef USE_TEXTURE
    setCubeSampling( getCubeSampling(true,true,0) );
#endif
}


bool MPEDisplay::allowsPicks() const
{
#ifndef USE_TEXTURE
    return true;
#else
    return false;
#endif
}


void MPEDisplay::turnOnSlice( bool yn )
{
#ifndef USE_TEXTURE
    if ( slices_.size() && slices_[dim_] )
	slices_[dim_]->turnOn( yn );
    /*if ( channels_[0] )
      channels_[0].turnOn( yn );*/
#endif
}


bool MPEDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
#ifndef USE_TEXTURE
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,MPEDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,MPEDisplay,dataTransformCB) );
    }

    return true;
#else
    return false;
#endif
}


const ZAxisTransform* MPEDisplay::getZAxisTransform() const
{ 
#ifndef USE_TEXTURE
    return datatransform_; 
#else
    return 0;
#endif
}


void MPEDisplay::dataTransformCB( CallBacker* )
{
#ifndef USE_TEXTURE
    updateRanges( false, true );
    if ( volumecache_) 
	setDataVolume( 0, volumecache_, 0 );
#endif
}

void MPEDisplay::triggerSel()
{
    updateMouseCursorCB( 0 );
    visBase::VisualObject::triggerSel();
}


void MPEDisplay::triggerDeSel()
{
    updateMouseCursorCB( 0 ); 
    visBase::VisualObject::triggerDeSel();
}


void MPEDisplay::updateRanges( bool updateic, bool updatez )
{
#ifndef USE_TEXTURE
    if ( !datatransform_ ) return;
// to do: check!
    // to do: save session cs in usePar?
/*    if ( csfromsession_ != SI().sampling(true) )
	setCubeSampling( csfromsession_ );
    else*/
    {
	Interval<float> zrg = datatransform_->getZInterval( false );
	CubeSampling cs = getCubeSampling( 0 );
	assign( cs.zrg, zrg );
	setCubeSampling( cs );
    }
#endif
}


void MPEDisplay::setChannels2VolData( visBase::TextureChannel2VolData* t )
{
    RefMan<visBase::TextureChannel2VolData> dummy( t );
    if ( !channels_ ) return;

    channels_->setChannels2RGBA( t );
}


visBase::TextureChannel2VolData* MPEDisplay::getChannels2VolData()
{ 
    return channels_ ? dynamic_cast<visBase::TextureChannel2VolData*> 
	(channels_->getChannels2RGBA()) : 0; 
}


void MPEDisplay::clearTextures()
{
    Attrib::SelSpec as;
    setSelSpec( 0, as );

    // to do: check!
    for ( int idy=channels_->nrVersions( 0 ) - 1; idy>=0; idy-- )
	channels_->setUnMappedData( 0, idy, 0, OD::UsePtr, 0 );	
    //channels_->reMapData( 0, 0 );
}


SurveyObject::AttribFormat MPEDisplay::getAttributeFormat( int attrib ) const
{
    return !attrib ? SurveyObject::Cube : SurveyObject::None;
}


int MPEDisplay::nrAttribs() const
{
    return ( as_.id() == Attrib::SelSpec::cNoAttrib() ) ? 0 : 1;
}


bool MPEDisplay::canAddAttrib( int nr ) const
{
    return ( nr + nrAttribs() <= 1 ) ? true : false;
}


bool MPEDisplay::canRemoveAttrib() const
{
    return ( nrAttribs() == 1 ) ? true : false;
}


bool MPEDisplay::addAttrib()
{
    BufferStringSet* aatrnms = new BufferStringSet();
    aatrnms->allowNull();
    userrefs_ += aatrnms;
    as_.set( "", Attrib::SelSpec::cAttribNotSel(), false, 0 );
    channels_->addChannel();
//    updateMainSwitch();
    return true;
}


bool MPEDisplay::removeAttrib( int attrib )
{
    channels_->removeChannel( attrib );
    as_.set( "", Attrib::SelSpec::cNoAttrib(), false, 0 );
    userrefs_.remove( attrib );
//    updateMainSwitch();
    return true;
}


bool MPEDisplay::isAttribEnabled( int attrib ) const
{
    return attrib ? false : channels_->getChannels2RGBA()->isEnabled( attrib );
}


void MPEDisplay::enableAttrib( int attrib, bool yn )
{
    if ( !attrib )
	channels_->getChannels2RGBA()->setEnabled( attrib, yn );
//    updateMainSwitch();
}

}; // namespace vissurvey

