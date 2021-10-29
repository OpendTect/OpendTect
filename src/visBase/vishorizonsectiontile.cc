 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/


#include "vishorizonsectiontile.h"
#include "vishortileresolutiondata.h"
#include "vishorizonsectiontileglue.h"


#include "threadwork.h"
#include "viscoord.h"
#include "vishorthreadworks.h"
#include "survinfo.h"
#include "binidsurface.h"

#include <osgGeo/LayeredTexture>

#include <osg/Switch>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osgUtil/CullVisitor>
#include <osg/UserDataContainer>

namespace visBase
{

HorizonSectionTile::HorizonSectionTile( const HorizonSection& section,
					const RowCol& origin )
    : osgswitchnode_( new osg::Switch )
    , desiredresolution_( cNoneResolution )
    , resolutionhaschanged_( false )
    , needsupdatebbox_( false )
    , nrdefinedvertices_( 0 )
    , origin_( origin )
    , hrsection_( section )
    , updatenewpoint_( false )
    , wireframedisplayed_( false )
    , righttileglue_( new HorizonSectionTileGlue )
    , bottomtileglue_( new HorizonSectionTileGlue )
    , stateset_( new osg::StateSet )
    , txorigin_( osg::Vec2f(0,0) )
    , txoppsite_( osg::Vec2f(0,0) )
    , normals_( new osg::Vec3Array )
    , osgvertices_( new osg::Vec3Array )
    , tesselationqueueid_( Threads::WorkManager::twm().addQueue(
	Threads::WorkManager::MultiThread, "Tessalation" ) )
    , cosanglexinl_( cos(SI().angleXInl()) )
    , sinanglexinl_( sin(SI().angleXInl()) )
{
    normals_->ref();
    osgvertices_->ref();
    refOsgPtr( stateset_ );
    refOsgPtr( osgswitchnode_ );
    tileresolutiondata_.allowNull();
    buildOsgGeometries();
    bbox_.init();
}


void HorizonSectionTile::buildOsgGeometries()
{
    for ( int i = LEFTUPTILE; i <= RIGHTBOTTOMTILE; i++ )
	neighbors_[i] = 0;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_ += new TileResolutionData( this, res );
	osgswitchnode_->addChild( tileresolutiondata_[res]->osgswitch_ );
    }

    osgswitchnode_->addChild( righttileglue_->getGeode() );
    osgswitchnode_->addChild( bottomtileglue_->getGeode() );

    initvertices();
    setLineWidth( hrsection_.getLineWidth() );
}


void HorizonSectionTile::turnOnGlue( bool yn )
{
    osgswitchnode_->setChildValue( righttileglue_->getGeode(), yn );
    osgswitchnode_->setChildValue( bottomtileglue_->getGeode(), yn );
}


void HorizonSectionTile::initvertices()
{
    const int coordsize =
	hrsection_.nrcoordspertileside_*hrsection_.nrcoordspertileside_;
    osg::Vec3Array* normals = mGetOsgVec3Arr( normals_ );
    normals->resize( coordsize );
    std::fill( normals->begin(),normals->end(),osg::Vec3f(0,0,-1) );
    osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );
    osgvertices->resize( coordsize );
}


HorizonSectionTile::~HorizonSectionTile()
{
    osgswitchnode_->removeChildren( 0, osgswitchnode_->getNumChildren() );
    unRefOsgPtr( osgswitchnode_ );
    unRefOsgPtr( stateset_ );

    if ( righttileglue_ ) delete righttileglue_;
    if ( bottomtileglue_ ) delete bottomtileglue_;

    Threads::WorkManager::twm().removeQueue( tesselationqueueid_, false );
    deepErase( tileresolutiondata_ );
    normals_->unref();
    osgvertices_->unref();
    setNrTexCoordLayers( 0 );
}


void HorizonSectionTile::setResolution( char res )
{
    desiredresolution_ = res;
}


char HorizonSectionTile::getActualResolution() const
{
    const int numchildren = osgswitchnode_->getNumChildren();
    for ( int i=0; i<numchildren-2; i++ ) // the last two children are glues
    {
	if ( osgswitchnode_->getValue( i ) )
	    return  tileresolutiondata_[i]->resolution_;
    }
    return cNoneResolution;
}


void HorizonSectionTile::updateAutoResolution( const osg::CullStack* cs )
{
    char newres = desiredresolution_;
    if ( newres==cNoneResolution )
    {
	updateBBox();
	if ( !bbox_.valid() )
	    newres = cNoneResolution;
	else if ( desiredresolution_==cNoneResolution )
	{
	    const char wantedres = getAutoResolution( cs );

	    if ( !tileresolutiondata_.validIdx( wantedres ) )
		return;
	    newres = wantedres;
	    datalock_.lock();
	    for ( ; newres<hrsection_.nrhorsectnrres_-1; newres++ )
	    {
		if ( tileresolutiondata_[newres]->needsretesselation_ <
		    cMustRetesselate )
		    break;
	    }
	    datalock_.unLock();

	    if ( !hrsection_.tesselationlock_ && ( wantedres!=newres ||
		tileresolutiondata_[newres]->needsretesselation_ ) )
	    {
		addTileTesselator( wantedres );
	    }
	}
    }

    setActualResolution( newres );
}




void HorizonSectionTile::addTileTesselator( int res )
{
    TileTesselator* tt = new TileTesselator( this, res );
    Threads::WorkManager::twm().addWork(
	Threads::Work( *tt, true ),
	0, tesselationqueueid_, true );
}


void HorizonSectionTile::addTileGlueTesselator()
{
    if ( !glueneedsretesselation_ ) return;

    TileGlueTesselator* tt = new TileGlueTesselator( this );
    Threads::WorkManager::twm().addWork(
	Threads::Work( *tt, true ),
	0, tesselationqueueid_, true );
}


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ )
	return;

    bbox_.init();
    osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );
    for ( int idx =0; idx<osgvertices->size(); idx++ )
    {
	if ( mIsOsgVec3Def((*osgvertices)[idx]) )
	   bbox_.expandBy( (*osgvertices)[idx] );
    }

    needsupdatebbox_ = false;
}


void HorizonSectionTile::enableGeometryTypeDisplay( GeometryType type, bool yn )
{
    wireframedisplayed_ = ( ( type== WireFrame ) && yn ) ? true : false;
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	if ( !tileresolutiondata_.validIdx(res) )
	    continue;

	TileResolutionData* tileres = tileresolutiondata_[res];
	if ( tileres )
	    tileres->enableGeometryTypeDisplay( type, yn );
    }
}


void HorizonSectionTile::setWireframeColor( Color& color )
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	if ( !tileresolutiondata_.validIdx(res) )
	    continue;

	TileResolutionData* tileres = tileresolutiondata_[res];
	if ( tileres )
	    tileres->setWireframeColor( color );
    }
}


void HorizonSectionTile::setLineWidth( int width )
{
    for ( int idx=0; idx<tileresolutiondata_.size(); idx++ )
    {
	if ( tileresolutiondata_[idx] )
	    tileresolutiondata_[idx]->setLineWidth( width );
    }
}


void HorizonSectionTile::ensureGlueTesselated()
{
    bool needgluetesselation =
	glueneedsretesselation_ || resolutionhaschanged_ ||
	(neighbors_[RIGHTTILE] &&
	neighbors_[RIGHTTILE]->resolutionhaschanged_) ||
	(neighbors_[BOTTOMTILE] &&
	neighbors_[BOTTOMTILE]->resolutionhaschanged_);

    if ( !needgluetesselation )
	return;

    const char res = getActualResolution();
    if( res==cNoneResolution ||
	( !neighbors_[RIGHTTILE] && !neighbors_[BOTTOMTILE] ) )
	return;

    for ( int nb=RIGHTTILE; nb<RIGHTBOTTOMTILE; nb += 2 )
    {
	bool isright = nb == RIGHTTILE ? true : false;

	HorizonSectionTileGlue* tileglue =
	    isright ? righttileglue_ : bottomtileglue_;
	if ( tileglue )
	{
	    turnOnGlue( false );
	    datalock_.lock();
	    tileglue->setDisplayTransformation( hrsection_.transformation_ );
	    tileglue->buildGlue( this, neighbors_[nb], isright );
	    datalock_.unLock();
	    turnOnGlue( true );
	}

    }

    glueneedsretesselation_ = false;

}


char HorizonSectionTile::getAutoResolution( const osg::CullStack* cs )
{
    const int cIdealNrPixelsPerCell = 32;

    updateBBox();
    if ( !bbox_.valid() || !cs ) return cNoneResolution;

    osg::Vec2 screensize;
    const float boxwidth = bbox_.xMax()-bbox_.xMin();
    const float boxheight = bbox_.yMax()-bbox_.yMin();

    screensize[0] = cs->clampedPixelSize( bbox_.center(),boxwidth );
    screensize[1] = cs->clampedPixelSize( bbox_.center(),boxheight );

    const int wantednumcells =
	(int)( screensize[0]*screensize[1]/cIdealNrPixelsPerCell );

    if ( !wantednumcells )
	return hrsection_.lowestresidx_;

    if ( nrdefinedvertices_<=wantednumcells )
	return 0;

    char maxres = nrdefinedvertices_/wantednumcells;
    if ( nrdefinedvertices_%wantednumcells ) maxres++;

    for ( int desiredres=hrsection_.lowestresidx_; desiredres>=0; desiredres-- )
    {
	if ( hrsection_.nrcells_[desiredres]>=wantednumcells )
	    return mMIN(desiredres,maxres);
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( char resolution )
{
    if ( resolution != getActualResolution() )
    {
	if ( resolution == cNoneResolution )
	    resolution = hrsection_.lowestresidx_;
	osgswitchnode_->setAllChildrenOff();
	osgswitchnode_->setValue( resolution, true );

	if ( !wireframedisplayed_ )
	{
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-1, true);
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-2, true);
	}
	else
	{
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-1, false);
	    osgswitchnode_->setValue(osgswitchnode_->getNumChildren()-2, false);
	}

	resolutionhaschanged_ = true;
    }

}


void HorizonSectionTile::tesselateResolution( char res, bool onlyifabsness )
{
    if ( !tileresolutiondata_.validIdx( res ) )
	return;

    datalock_.lock();

    if ( updatenewpoint_ )
    {
	for ( int idx=0; idx<=hrsection_.lowestresidx_; idx++ )
	    tileresolutiondata_[idx]->tesselateResolution( false );
	updatenewpoint_ =  false;
    }
    else
	tileresolutiondata_[res]->tesselateResolution( onlyifabsness );

    datalock_.unLock();
}


void HorizonSectionTile::applyTesselation( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) )
	return;
    osgswitchnode_->setChildValue( osgswitchnode_->getChild( res ), true );
    resolutionhaschanged_ = false;
}


void HorizonSectionTile::dirtyGeometry()
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_[res]->dirtyGeometry();
    }

}


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    nrdefinedvertices_ = 0;
    datalock_.lock();

    const int nrcoords = hrsection_.nrcoordspertileside_;
    ConstRefMan<Transformation> trans = hrsection_.transformation_;

    int crdidx = 0;
    bbox_.init();
    osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );
    if ( osgvertices->size()<nrcoords*nrcoords )
	osgvertices->resize( nrcoords*nrcoords );

    for ( int row=0; row<nrcoords; row++ )
    {
	for ( int col=0; col<nrcoords; col++ )
	{
	    int coordidx = col + row*nrcoords;
	    Coord3 vertex = pos[coordidx];
	    const int size = pos.size();
	    if ( coordidx >= size || !vertex.isDefined() )
	    {
		vertex[2] = mUdf(float);
	    }
	    else
	    {
		if ( trans )
		    trans->transform( vertex );
		nrdefinedvertices_ ++;
	    }

	    (*osgvertices)[crdidx] = Conv::to<osg::Vec3f>( vertex );

	    if ( vertex[2] != mUdf(float) )
	    {
		bbox_.expandBy( (*osgvertices)[crdidx] );
		computeNormal( crdidx,(*mGetOsgVec3Arr(normals_))[crdidx] );
	    }
	    crdidx++;
       }
    }

    datalock_.unLock();
    needsupdatebbox_ = false;
}


void HorizonSectionTile::updatePrimitiveSets()
{
    const char res = getActualResolution();
    if ( !tileresolutiondata_.validIdx( res ) )
	return;
    datalock_.lock();
    tileresolutiondata_[res]->updatePrimitiveSets();
    datalock_.unLock();
}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{
    neighbors_[nbidx] = nb;
    glueneedsretesselation_ = true;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
    if ( row >=0 && row <hrsection_.nrcoordspertileside_ &&
	 col>=0 && col <hrsection_.nrcoordspertileside_ )
    {

	datalock_.lock();

	const int coordidx = row*hrsection_.nrcoordspertileside_+ col;

	osg::Vec3Array* arr = mGetOsgVec3Arr( osgvertices_ );

	const bool olddefined = mIsOsgVec3Def((*arr)[coordidx]);
	const bool newdefined = pos.isDefined();

	if ( !newdefined && olddefined )
	    nrdefinedvertices_--;
	else if ( newdefined && !olddefined )
	    nrdefinedvertices_++;

	if ( !newdefined )
	{
	    //it is going to assign a undefined point to vertex,first we should
	    // hide osg render.
	    char curres = getActualResolution();
	    tileresolutiondata_[curres]->hideFromDisplay();
	    (*arr)[coordidx][2] = mUdf(float);
	}
	else
	{
	    Coord3 crd;
	    mVisTrans::transform( hrsection_.transformation_, pos, crd );
	    (*arr)[coordidx] = Conv::to<osg::Vec3f>(crd);
	    bbox_.expandBy( (*arr)[coordidx] );
	    computeNormal( coordidx,( *mGetOsgVec3Arr(normals_) )[coordidx] );
	}

	for ( int res=0; res<=hrsection_.lowestresidx_; res++ )
	    tileresolutiondata_[res]->needsretesselation_ = cShouldRetesselate;

	datalock_.unLock();

	needsupdatebbox_ = true;
	glueneedsretesselation_ = true;
	updatenewpoint_ = true;
    }

}


void HorizonSectionTile::setTexture( const Coord& origincrd,
    const Coord& oppositecrd )
{
    txorigin_ = Conv::to<osg::Vec2f>( origincrd );
    txoppsite_ = Conv::to<osg::Vec2f>( oppositecrd );

    if ( (txoppsite_-txorigin_)==osg::Vec2f(0.0, 0.0) )
	return;

    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;
    const osgGeo::LayeredTexture* texture = hrsection_.getOsgTexture();

    unRefOsgPtr( stateset_ );
    stateset_ = texture->createCutoutStateSet( txorigin_, txoppsite_, tcdata );
    refOsgPtr( stateset_ );

    const bool eachlayersametexcoords = true;	/* Performance shortcut */

    int nrtexcoordlayers = tcdata.size();
    if ( eachlayersametexcoords && nrtexcoordlayers>1 )
	nrtexcoordlayers = 1;

    txunits_.erase();
    setNrTexCoordLayers( nrtexcoordlayers );

    std::vector<osgGeo::LayeredTexture::TextureCoordData>::iterator tcit =
								tcdata.begin();
    for ( int layeridx=0; tcit!=tcdata.end(); tcit++, layeridx++ )
    {
	const int tcidx = mMIN( layeridx, nrtexcoordlayers-1 );
	const osg::Vec2 texturetilestep = tcit->_tc11 - tcit->_tc00;

	osg::Vec2Array* txcoords = mGetOsgVec2Arr( txcoords_[tcidx] );

	if ( layeridx == tcidx )
	{
	    const osg::Vec2f diff = txoppsite_-txorigin_;
	    const int size = hrsection_.nrcoordspertileside_;
	    for ( int r=0; r<size; r++ )
	    {
		for ( int c=0; c<size; c++ )
		{
		    (*txcoords)[r*size+c][0] = tcit->_tc00[0] +
			    ( float(c)/diff.x() )*texturetilestep.x();
		    (*txcoords)[r*size+c][1] = tcit->_tc00[1] +
				    ( float(r)/diff.y() )*texturetilestep.y();
		 }
	    }

	}

	for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	{
	    tileresolutiondata_[res]->setTexture( tcit->_textureUnit,
						  txcoords, stateset_ );
	}

	txunits_ += tcit->_textureUnit;
    }
}


void HorizonSectionTile::setNrTexCoordLayers( int nrlayers )
{
    while ( nrlayers > txcoords_.size() )
    {
	txcoords_.push_back( new osg::Vec2Array );
	txcoords_.back()->ref();
    }
    while ( nrlayers < txcoords_.size() )
    {
	txcoords_.back()->unref();
	txcoords_.pop_back();
    }

    initTexCoordLayers();
}


void HorizonSectionTile::initTexCoordLayers()
{
    const int coordsize =
	hrsection_.nrcoordspertileside_*hrsection_.nrcoordspertileside_;

    for ( int idx = 0; idx<txcoords_.size(); idx++ )
	mGetOsgVec2Arr( txcoords_[idx] )->resize( coordsize );
}


bool HorizonSectionTile::hasDefinedCoordinates( int idx ) const
{
    const osg::Vec3Array* osgvertices = mGetOsgVec3Arr( osgvertices_ );
    if ( osgvertices )
	return mIsOsgVec3Def( (*osgvertices)[idx] );

    return false;
}


#define mExtractPosition(flag,location) \
{ \
    const Coord3 thispos = hrsection_.geometry_->getKnot(arrpos,false); \
    if ( thispos.isDefined() ) \
    { \
	Coord3 crd; \
	mVisTrans::transform( hrsection_.transformation_, thispos, crd ); \
	location##pos.x = ( flag ? -1 : 1 ) * idx * rcdist; \
	location##pos.y = crd.z; \
	location##found = true; \
    } \
}


double HorizonSectionTile::calcGradient( int row, int col,
    const StepInterval<int>& rcrange, bool isrow )
{
    const float rcdist =
	isrow ? hrsection_.rowdistance_ : hrsection_.coldistance_;
    const int rc = isrow ? row : col;
    bool beforefound = false; bool afterfound = false;
    Coord beforepos, afterpos;

    for ( int idx=hrsection_.spacing_[0]; idx>=0; idx-- )
    {
	if ( !beforefound )
	{
	    const int currc = rc - idx*rcrange.step;
	    if ( currc >= rcrange.start )
	    {
		const RowCol arrpos = isrow ?
		    RowCol( currc, col ) : RowCol( row, currc );
		mExtractPosition( true, before );
	    }
	}

	if ( idx>0 && !afterfound )
	{
	    const int currc = rc + idx*rcrange.step;
	    if ( currc <= rcrange.stop )
	    {
		const RowCol arrpos = isrow ?
		    RowCol( currc, col ) : RowCol( row, currc );
		mExtractPosition( false, after );
	    }
	}

	if ( afterfound && beforefound )
	    break;
    }

    return !afterfound || !beforefound ? 0
	: (afterpos.y-beforepos.y)/(afterpos.x-beforepos.x);
}


void HorizonSectionTile::computeNormal( int nmidx,osg::Vec3& normal )
{
    if ( !hrsection_.geometry_ )
	return;

    RowCol step;
    if ( hrsection_.userchangedisplayrg_ )
	step = RowCol(hrsection_.displayrrg_.step,hrsection_.displaycrg_.step);
    else
	step = RowCol( hrsection_.geometry_->rowRange().step,
		       hrsection_.geometry_->colRange().step );

    const int normalrow = (nmidx-hrsection_.normalstartidx_[0])/
	hrsection_.normalsidesize_[0];
    const int normalcol = (nmidx-hrsection_.normalstartidx_[0])%
	hrsection_.normalsidesize_[0];

    const int row = origin_.row() + step.row() * normalrow*
	hrsection_.spacing_[0];
    const int col = origin_.col() + step.col() * normalcol*
	hrsection_.spacing_[0];

    const double drow=
	calcGradient( row, col, hrsection_.geometry_->rowRange(), true );
    const double dcol=
	calcGradient( row, col, hrsection_.geometry_->colRange(), false );

    osg::Vec3 osgnormal;
    osgnormal[0] = drow*cosanglexinl_+dcol*sinanglexinl_;
    osgnormal[1] = dcol*cosanglexinl_-drow*sinanglexinl_;
    osgnormal[2] = -1;

    if ( mIsOsgVec3Def( osgnormal ) )
	normal = osgnormal;
}


const HorizonSectionTile* HorizonSectionTile::getNeighborTile( int idx ) const
{
    return idx>=0 && idx < 9 ? neighbors_[idx] : nullptr;
}


bool HorizonSectionTile::getResolutionCoordinates(
    TypeSet<Coord3>& coords) const
{
    const osg::Vec3Array* osgarr = mGetOsgVec3Arr( osgvertices_ );
    if ( !osgarr ) return false;

    coords.setEmpty();
    for ( int idx = 0; idx<osgarr->size(); idx++ )
	coords += Conv::to<Coord3>( (*osgarr)[idx] );

    return true;
}


bool HorizonSectionTile::getResolutionNormals(TypeSet<Coord3>& coords) const
{
    const osg::Vec3Array* arr = mGetOsgVec3Arr( normals_ );
    if ( !arr ) return false;

    coords.setEmpty();
    for ( int idx = 0; idx<arr->size(); idx++ )
	coords += Coord3( (*arr)[idx].x(), (*arr)[idx].y(), (*arr)[idx].z() );

    return true;
}


bool HorizonSectionTile::getResolutionTextureCoordinates(
    TypeSet<Coord>& coords) const
{
//    if ( txunits_.size()==0 ) return false;

    coords.setEmpty();

    osgGeo::LayeredTexture* entiretxture = hrsection_.getOsgTexture();
    const osg::Image*	entireimg = entiretxture->getCompositeTextureImage();

    const Coord entireorigin = Coord( 0.5/entireimg->s(), 0.5/entireimg->t() );
    const int nrcoords = hrsection_.nrcoordspertileside_;

    Coord offset;
    offset.x = txorigin_[0]/entireimg->s() + entireorigin.x;
    offset.y = txorigin_[1]/entireimg->t() + entireorigin.y;

    for ( int y=0; y<nrcoords; y++ )
    {
	for ( int x=0; x<nrcoords; x++ )
	{
	    const Coord txcrd = Coord( (double)x/entireimg->s(),
		(double)y/entireimg->t() ) + offset;
	    coords += txcrd;
	}
    }

    return true;
}


bool HorizonSectionTile::getResolutionPrimitiveSet(
    TypeSet<int>& ps,GeometryType type) const
{
    const char res = getActualResolution();
    return getResolutionPrimitiveSet( res, ps, type );
}


bool HorizonSectionTile::getResolutionPrimitiveSet(
    char res, TypeSet<int>& ps,GeometryType type) const
{
    if ( res==-1 ) res = 0;

    const osg::PrimitiveSet* osgps =
	tileresolutiondata_[res]->getPrimitiveSet( type );

    if ( !osgps ) return false;

    for ( int idx = 0; idx<osgps->getNumIndices(); idx++ )
	ps += osgps->index( idx );

    return true;
}

} // namespace visBase
