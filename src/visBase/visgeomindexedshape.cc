/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visgeomindexedshape.h"

#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "indexedshape.h"
#include "viscoord.h"
#include "visforegroundlifter.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexturecoords.h"
#include "SoIndexedTriangleFanSet.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/SoDB.h>

#include "SoIndexedLineSet3D.h"

#define mNrMaterials		256
#define mNrMaterialSteps	255
#define mUndefMaterial		255

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl( true )
    , coords_( Coordinates::create() )
    , normals_( Normals::create() )
    , texturecoords_( TextureCoords::create() )				   
    , shape_( 0 )
    , lineradius_( -1 )
    , lineconstantonscreen_( false )
    , linemaxsize_( -1 )
    , hints_( new SoShapeHints )			
    , ctab_( 0 )
    , lifter_( ForegroundLifter::create() )	
    , lifterswitch_( new SoSwitch )						
{
    lifter_->ref();
    lifter_->setLift(0.8);
    lifterswitch_->ref();
    lifterswitch_->addChild( lifter_->getInventorNode() );
    lifterswitch_->whichChild = SO_SWITCH_NONE;
    addChild( lifterswitch_ );

    addChild( hints_ );

    setLockable();
    coords_->ref();
    addChild( coords_->getInventorNode() );

    normals_->ref();
    addChild( normals_->getInventorNode() );

    texturecoords_->ref();
    addChild( texturecoords_->getInventorNode() );

    if ( getMaterial() )
	getMaterial()->change.notify( mCB(this,GeomIndexedShape,matChangeCB) );

    renderOneSide( 0 );
}


GeomIndexedShape::~GeomIndexedShape()
{
    lifter_->unRef();
    coords_->unRef();
    normals_->unRef();
    texturecoords_->unRef();
    delete ctab_;

    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,GeomIndexedShape,matChangeCB) );
}


GeomIndexedShape::ColTabMaterial::ColTabMaterial()
    : coltab_( visBase::Material::create() )
    , cache_( 0 )
    , materialbinding_( new SoMaterialBinding )
{
    materialbinding_->ref();
    materialbinding_->value = SoMaterialBinding::PER_VERTEX_INDEXED;

    coltab_->ref();
}


GeomIndexedShape::ColTabMaterial::~ColTabMaterial()
{
    coltab_->unRef();
    materialbinding_->unref();
}


void GeomIndexedShape::ColTabMaterial::updatePropertiesFrom( const Material* m )
{
    const float diffintensity = m->getDiffIntensity( 0 );
    const float transparency = m->getTransparency( 0 );
    for ( int idx=0; idx<mNrMaterials; idx++ )
    {
	coltab_->setDiffIntensity( diffintensity, idx );
	coltab_->setTransparency( transparency, idx );
    }

    coltab_->setAmbience( m->getAmbience() );
    coltab_->setSpecIntensity( m->getSpecIntensity() );
    coltab_->setEmmIntensity( m->getEmmIntensity() );
    coltab_->setShininess( m->getShininess() );
}


void GeomIndexedShape::turnOnForegroundLifter( bool yn )
{ lifterswitch_->whichChild = yn ? 0 : SO_SWITCH_NONE; }


void GeomIndexedShape::renderOneSide( int side )
{
    hints_->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    
    if ( side==0 )
    {
	hints_->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    }
    else if ( side==1 )
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
    else
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
}


void GeomIndexedShape::setMaterial( Material* mat )
{
    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,GeomIndexedShape,matChangeCB) );

    VisualObjectImpl::setMaterial( mat );
    if ( !mat || !ctab_ )
	return;

    ctab_->updatePropertiesFrom ( mat );

    mat->change.notify( mCB(this,GeomIndexedShape,matChangeCB) );
}

void GeomIndexedShape::updateMaterialFrom( const Material* mat )
{
    if ( ctab_ )
	ctab_->updatePropertiesFrom ( mat );
}


void GeomIndexedShape::matChangeCB( CallBacker* )
{
    updateMaterialFrom( getMaterial() );
}



void GeomIndexedShape::createColTab()
{
    if ( !ctab_ )
	ctab_ = new ColTabMaterial;

    if ( getMaterial() )
	ctab_->updatePropertiesFrom ( getMaterial() );
}


void GeomIndexedShape::enableColTab( bool yn )
{
    if ( yn )
	createColTab();

    if ( yn )
    {
	insertChild( childIndex(coords_->getInventorNode()),
		ctab_->coltab_->getInventorNode() );
	insertChild( childIndex(coords_->getInventorNode()),
		ctab_->materialbinding_ );
    }
    else if ( ctab_ )
    {
	removeChild( ctab_->coltab_->getInventorNode() );
	removeChild( ctab_->materialbinding_ );
    }
}


bool GeomIndexedShape::isColTabEnabled() const
{
    return ctab_ && childIndex( ctab_->coltab_->getInventorNode() )!=-1;
}


void GeomIndexedShape::setDataMapper( const ColTab::MapperSetup& setup,
				      TaskRunner* tr )
{
    createColTab();
    if ( setup!=ctab_->mapper_.setup_ )
    {
	ctab_->mapper_.setup_ = setup;
	if ( setup.type_!=ColTab::MapperSetup::Fixed )
	    reClip();
	reMap( tr );
    }
}


const ColTab::MapperSetup* GeomIndexedShape::getDataMapper() const
{ return ctab_ ? &ctab_->mapper_.setup_ : 0; }


void GeomIndexedShape::setDataSequence( const ColTab::Sequence& seq )
{
    createColTab();
    if ( seq!=ctab_->sequence_ )
    {
	ctab_->sequence_ = seq;
	for ( int idx=0; idx<mNrMaterialSteps; idx++ )
	{
	    const float val = ((float) idx)/(mNrMaterialSteps-1);
	    const Color col = seq.color( val );
	    ctab_->coltab_->setColor( col, idx+1 );
	}

	ctab_->coltab_->setColor( seq.undefColor(), mUndefMaterial+1 );
    }
}


const ColTab::Sequence* GeomIndexedShape::getDataSequence() const
{ return ctab_ ? &ctab_->sequence_ : 0; }


void GeomIndexedShape::setDisplayTransformation( const mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
    normals_->setDisplayTransformation( nt );
}


const mVisTrans* GeomIndexedShape::getDisplayTransformation() const
{ return coords_->getDisplayTransformation(); }


void GeomIndexedShape::setRightHandSystem( bool yn )
{
    if ( yn!=righthandsystem_ )
	normals_->inverse();

    VisualObjectImpl::setRightHandSystem( yn );
    if ( shape_ ) shape_->setRightHandedNormals( yn );

    //for ( int idx=lines_.size()-1; idx>=0; idx-- )
    //{
	//mDynamicCastGet( SoIndexedLineSet3D*, line3d, lines_[idx] );
	//if ( !line3d )
	    //continue;
//
	//line3d->rightHandSystem = righthandsystem_;
    //}
}


void GeomIndexedShape::setSurface( Geometry::IndexedShape* ns, TaskRunner* tr )
{
    shape_ = ns;
    shape_->setCoordList( new CoordListAdapter(*coords_),
	    		  new NormalListAdapter(*normals_), 
			  new TextureCoordListAdapter(*texturecoords_) );
    shape_->setRightHandedNormals( righthandsystem_ );
    touch( false, tr );
}


#define mHandleType( type, SoObj, list ) \
if ( geom->type_==Geometry::IndexedGeometry::type ) \
{ \
    const int idy = list##geoms_.indexOf( geom ); \
    if ( idy==-1 || !dynamic_cast<SoObj*>(list##s_[idy]) ) \
    { \
	shape = new SoObj; \
	addChild( shape ); \
    } \
    else \
    { \
	shape = list##s_[idy]; \
	list##s_.remove( idy ); \
	list##geoms_.remove( idy ); \
    } \
 \
    new##list##s += shape; \
    new##list##geoms += geom; \
}


#define mRemoveOld( list ) \
    while ( list##s_.size() ) \
    { \
	SoIndexedShape* shape = list##s_.remove(0); \
 \
	const int idx = childIndex( shape ); \
	mDynamicCastGet(SoNormalBinding*, nb, idx>0 ? getChild(idx-1) : 0); \
	if ( nb ) removeChild( nb ); \
	removeChild( shape ); \
    } \
 \
    list##geoms_.erase(); \
 \
    list##s_ = new##list##s; \
    list##geoms_ = new##list##geoms

void GeomIndexedShape::touch( bool forall, TaskRunner* tr )
{
    if ( !tryWriteLock() )
    {
	pErrMsg("Could not lock");
	return;
    }

    if ( shape_ && shape_->needsUpdate() )
	shape_->update( forall, tr );

    ObjectSet<SoIndexedShape> newstrips;
    ObjectSet<const Geometry::IndexedGeometry> newstripgeoms;

    ObjectSet<SoIndexedShape> newlines;
    ObjectSet<const Geometry::IndexedGeometry> newlinegeoms;

    ObjectSet<SoIndexedShape> newfans;
    ObjectSet<const Geometry::IndexedGeometry> newfangeoms;

    if ( !shape_ )
    {
	mRemoveOld( strip );
	mRemoveOld( fan );
	mRemoveOld( line );

	writeUnLock();
	return;
    }

    const ObjectSet<Geometry::IndexedGeometry>& geoms=shape_->getGeometry();

    for ( int idx=0; idx<geoms.size(); idx++ )
    {
	const Geometry::IndexedGeometry* geom = geoms[idx];
	SoIndexedShape* shape = 0;
	mHandleType( TriangleStrip, SoIndexedTriangleStripSet, strip )
	else mHandleType( TriangleFan, SoIndexedTriangleFanSet, fan )
	else if ( lineradius_ >= 0 )
	{
	    mHandleType( Lines, SoIndexedLineSet3D, line );
	    mDynamicCastGet( SoIndexedLineSet3D*, line3d, shape );
	    if ( line3d )
	    {
		line3d->radius = lineradius_;
		line3d->screenSize = lineconstantonscreen_;
		line3d->maxRadius = linemaxsize_;
		//line3d->rightHandSystem = righthandsystem_;
	    }
	}
	else
	    mHandleType( Lines, SoIndexedLineSet, line )

	if ( !shape )
	    continue;

	if ( geom->ischanged_ )
	{
	    /* TODO: leads to crash. Probably because geom has been deleted.
	    shape->coordIndex.setValuesPointer(
		geom->coordindices_.size(), geom->coordindices_.arr() );

	    shape->normalIndex.setValuesPointer(
		geom->normalindices_.size(), geom->normalindices_.arr() );
	    */

	    SbBool oldstatus = shape->coordIndex.enableNotify( false );
	    shape->coordIndex.setValues( 0,
		geom->coordindices_.size(), geom->coordindices_.arr() );
	    shape->coordIndex.setNum( geom->coordindices_.size() );
	    shape->coordIndex.enableNotify( oldstatus );
	    shape->coordIndex.touch();

	    if ( shape_->createsNormals() )
	    {
    		oldstatus = shape->normalIndex.enableNotify( false );
    		shape->normalIndex.setValues( 0,
			geom->normalindices_.size(), 
			geom->normalindices_.arr() );
    		shape->normalIndex.setNum( geom->normalindices_.size() );
    		shape->normalIndex.enableNotify( oldstatus );
		shape->normalIndex.touch();
	    }
	    else
		shape->normalIndex.setNum( 0 );

	    if ( shape_->createsTextureCoords() )
	    {
		oldstatus = shape->textureCoordIndex.enableNotify( false );
		shape->textureCoordIndex.setValues( 0,
			geom->texturecoordindices_.size(), 
			geom->texturecoordindices_.arr() );
		shape->textureCoordIndex.setNum(
			geom->texturecoordindices_.size() );    
		shape->textureCoordIndex.enableNotify( oldstatus );
		shape->textureCoordIndex.touch();
	    }
	    else
		shape->textureCoordIndex.setNum( 0 );
	}

	const int idy = childIndex( shape );
	mDynamicCastGet(SoNormalBinding*, nb, idy>0 ? getChild(idy-1) : 0);

	if ( geom->normalindices_.size() )
	{
	    if ( !nb )
	    {
		nb = new SoNormalBinding;
		insertChild( idy, nb );
	    }

	    nb->value = geom->normalbinding_==
			     Geometry::IndexedGeometry::PerVertex 
		 ? SoNormalBindingElement::PER_VERTEX_INDEXED
		 : SoNormalBindingElement::PER_FACE_INDEXED;
	}
	else if ( nb )
	    removeChild( nb );

	geom->ischanged_ = false;
    }

    mRemoveOld( strip );
    mRemoveOld( fan );
    mRemoveOld( line );

    writeUnLock();
}


void GeomIndexedShape::getAttribPositions( DataPointSet& set,TaskRunner*) const
{
    const DataColDef coordindex( sKeyCoordIndex() );
    if ( set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact)==-1 )
	set.dataSet().add( new DataColDef(coordindex) );

    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    int coordid = -1;
    while ( true )
    {
	coordid = coords_->nextID( coordid );
	if ( coordid==-1 )
	    break;

	const Coord3 pos = coords_->getPos( coordid );
	DataPointSet::Pos dpsetpos( BinID(mNINT32(pos.x),mNINT32(pos.y)), 
							    (float) pos.z );
	DataPointSet::DataRow datarow( dpsetpos, 1 );
	datarow.data_.setSize( set.nrCols(), mUdf(float) );
	datarow.data_[col-set.nrFixedCols()] =  coordid;
	set.addRow( datarow );
    }

    set.dataChanged();
}
    

void GeomIndexedShape::setAttribData( const DataPointSet& set,TaskRunner* tr)
{
    createColTab();

    const DataColDef coordindex( sKeyCoordIndex() );
    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    if ( col==-1 )
	return;

    const BinIDValueSet& vals = set.bivSet();
    if ( vals.nrVals()<col+1 )
	return;

    ArrayValueSeries<float,float>& cache = ctab_->cache_;
    cache.setSize( vals.totalSize() );
    cache.setAll( mUdf(float) );

    BinIDValueSet::Pos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const int coordidx = mNINT32(ptr[col]);
	const float val = ptr[col+1];

	if ( coordidx>=cache.size() )
	{
	    int oldsz = cache.size();
	    cache.setSize( coordidx+1 );
	    if ( !cache.arr() )
		return;

	    const float udf = mUdf(float);
	    for ( int idx=oldsz; idx<=coordidx; idx++ )
		cache.setValue( idx, udf );
	}

	cache.setValue( coordidx, val );
    }

    if ( ctab_->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	reClip();
    reMap( tr );
}


void GeomIndexedShape::reMap( TaskRunner* tr )
{ 
    createColTab();
    if ( ctab_->cache_.size()<=0 )
	return;

    TypeSet<int> material( ctab_->cache_.size(), -1 );
    if ( !material.arr() )
	return;

    for ( int idx=0; idx<material.size(); idx++ )
    {
	material[idx] = ColTab::Mapper::snappedPosition( &ctab_->mapper_,
		ctab_->cache_[idx], mNrMaterialSteps, mUndefMaterial )+1;
    }

    for ( int idx=strips_.size()-1; idx>=0; idx-- )
    {
	const int numvals = strips_[idx]->coordIndex.getNum();
	const int* ciptr = strips_[idx]->coordIndex.getValues( 0 );
	strips_[idx]->materialIndex.setNum( numvals );

	mPointerOperation( int, strips_[idx]->materialIndex.startEditing(),
		= *ciptr==-1 ? -1 : material[*ciptr], numvals, ++; ciptr++ );

	strips_[idx]->materialIndex.finishEditing();
    }
}


void GeomIndexedShape::reClip()
{
    createColTab();
    ctab_->mapper_.setData( &ctab_->cache_, ctab_->cache_.size() );
}


void GeomIndexedShape::set3DLineRadius( float radius, bool constantonscreen,
					float maxworldsize )
{
    if ( lineradius_ != radius ||
	 lineconstantonscreen_ != constantonscreen ||
	 linemaxsize_ != maxworldsize )
    {
	lineradius_ = radius;
	lineconstantonscreen_ = constantonscreen;
	linemaxsize_ = maxworldsize;
	touch( true );
    }
}


}; // namespace visBase
