/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID mUnusedVar = "$Id: vismarchingcubessurfacedisplay.cc,v 1.49 2012-08-13 04:04:39 cvsaneesh Exp $";

#include "vismarchingcubessurfacedisplay.h"

#include "arrayndimpl.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "executor.h"
#include "indexedshape.h"
#include "impbodyplaneintersect.h"
#include "keystrs.h"
#include "marchingcubes.h"
#include "posvecdataset.h"
#include "randcolor.h"
#include "selector.h"
#include "settings.h"
#include "survinfo.h"
#include "visgeomindexedshape.h"
#include "vismarchingcubessurface.h"
#include "visplanedatadisplay.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::MarchingCubesDisplay );

namespace visSurvey
{

MarchingCubesDisplay::MarchingCubesDisplay()
    : VisualObjectImpl(true)
    , emsurface_( 0 )
    , displaysurface_( 0 )
    , impbody_( 0 )
    , displayintersections_( false )		   
{
    cache_.allowNull( true );
    setColor( getRandomColor( false ) );
    getMaterial()->setAmbience( 0.4 );
    getMaterial()->change.notify(
	    mCB(this,MarchingCubesDisplay,materialChangeCB));
}


MarchingCubesDisplay::~MarchingCubesDisplay()
{
    if ( emsurface_ ) emsurface_->unRef();
   
    delete impbody_;
    deepErase( intsinfo_ );

    if ( displaysurface_ )
	displaysurface_->unRef();

    getMaterial()->change.remove(
	    mCB(this,MarchingCubesDisplay,materialChangeCB));
    for ( int idx=cache_.size()-1; idx>=0; idx-- )
    {
	if ( !cache_[idx] )
	    continue;
	
	DPM( DataPackMgr::PointID() ).release( cache_[idx]->id() );
	delete cache_[idx];
    }
}


void MarchingCubesDisplay::useTexture( bool yn )
{
    if ( displaysurface_ )
    {
    	displaysurface_->getShape()->enableColTab( yn );
    }
}


bool MarchingCubesDisplay::usesTexture() const
{ 
    return displaysurface_ ? displaysurface_->getShape()->isColTabEnabled()
			   : false; 
}


bool MarchingCubesDisplay::setVisSurface(visBase::MarchingCubesSurface* surface)
{
    if ( displaysurface_ )
    {
	removeChild( displaysurface_->getInventorNode() );
	displaysurface_->unRef();
	displaysurface_ = 0;
    }
	
    if ( emsurface_ ) emsurface_->unRef();
    emsurface_ = 0;

    delete impbody_; impbody_ = 0;

    if ( !surface || !surface->getSurface() )
	return false;

    mTryAlloc( emsurface_, EM::MarchingCubesSurface( EM::EMM() ) );

    if ( !emsurface_ )
    {
	for ( int idx=0; idx<intsinfo_.size(); idx++ )
	    intsinfo_[idx]->visshape_->turnOn( false );
	return false;
    }

    emsurface_->ref();

    if ( !emsurface_->isOK() ||
	 !emsurface_->setSurface( surface->getSurface() ) )
    {
	emsurface_->unRef();
	emsurface_ = 0;
	return false;
    }

    SamplingData<float> sd = surface->getScale( 0 );
    emsurface_->setInlSampling(
	    SamplingData<int>( mNINT32(sd.start), mNINT32(sd.step) ) );

    sd = surface->getScale( 1 );
    emsurface_->setCrlSampling(
	    SamplingData<int>( mNINT32(sd.start), mNINT32(sd.step) ) );

    emsurface_->setZSampling( surface->getScale( 2 ) );

    EM::EMM().addObject( emsurface_ );

    displaysurface_ = surface;
    displaysurface_->ref();
    displaysurface_->setSelectable( false );
    displaysurface_->setRightHandSystem( righthandsystem_ );
    addChild( displaysurface_->getInventorNode() );

    if ( displaysurface_->getMaterial() )
	getMaterial()->setFrom( *displaysurface_->getMaterial() );

    displaysurface_->setMaterial( 0 );
    getMaterial()->change.notify(
	    mCB(this,MarchingCubesDisplay,materialChangeCB));
    emsurface_->setPreferredColor( getColor() );
    emsurface_->setName( name() );
    
    materialChangeCB( 0 );
    return true;
}


EM::ObjectID MarchingCubesDisplay::getEMID() const
{ return emsurface_ ? emsurface_->id() : -1; }


SurveyObject::AttribFormat MarchingCubesDisplay::getAttributeFormat(int) const
{ return SurveyObject::RandomPos; }


int MarchingCubesDisplay::nrAttribs() const
{ return 1; }


bool MarchingCubesDisplay::canAddAttrib(int) const
{ return false; }


bool MarchingCubesDisplay::canRemoveAttrib() const
{ return false; }


bool MarchingCubesDisplay::canHandleColTabSeqTrans(int) const
{ return false; }


const ColTab::MapperSetup*
MarchingCubesDisplay::getColTabMapperSetup( int attrib, int version ) const
{
    return !attrib && (!version || mIsUdf(version)) && displaysurface_
	? displaysurface_->getShape()->getDataMapper()
	: 0;
}


void MarchingCubesDisplay::setColTabMapperSetup( int attrib,
	const ColTab::MapperSetup& setup, TaskRunner* tr )
{
    if ( !attrib )
	displaysurface_->getShape()->setDataMapper( setup, tr );
}


const ColTab::Sequence*
MarchingCubesDisplay::getColTabSequence( int attrib ) const
{
    return !attrib && displaysurface_
	? displaysurface_->getShape()->getDataSequence()
	: 0;
}


void MarchingCubesDisplay::setColTabSequence( int attrib,
			      const ColTab::Sequence& seq, TaskRunner* )
{
    if ( !attrib && displaysurface_ )
	displaysurface_->getShape()->setDataSequence( seq );
}


bool MarchingCubesDisplay::canSetColTabSequence() const
{ return true; }


void MarchingCubesDisplay::setSelSpec( int attrib, const Attrib::SelSpec& spec )
{
    if ( !attrib )
	selspec_ = spec;
}


const Attrib::SelSpec* MarchingCubesDisplay::getSelSpec( int attrib ) const
{
    return attrib ? 0 : &selspec_;
}


#define mSetDataPointSet(nm) \
    selspec_.set( nm, Attrib::SelSpec::cNoAttrib(), false, "" ); \
    DataPointSet* data = new DataPointSet(false,true); \
    DPM( DataPackMgr::PointID() ).addAndObtain( data ); \
    getRandomPos( *data, 0 ); \
    DataColDef* isovdef = new DataColDef("Depth"); \
    data->dataSet().add( isovdef ); \
    BinIDValueSet& bivs = data->bivSet();  \
    if ( !data->size() || bivs.nrVals()!=3 ) \
    { DPM( DataPackMgr::PointID() ).release( data->id() ); return;} \
    int valcol = data->dataSet().findColDef( *isovdef, \
	    PosVecDataSet::NameExact ); \
    if ( valcol==-1 ) valcol = 1


void MarchingCubesDisplay::setIsoPatch( int attrib )
{
    mSetDataPointSet("Isopach");

    if ( !impbody_ ) impbody_ = emsurface_->createImplicitBody(0,false);
    if ( !impbody_ || !impbody_->arr_ ) return;

    const int inlsz = impbody_->cs_.nrInl();
    const int crlsz = impbody_->cs_.nrCrl();
    const int zsz = impbody_->cs_.nrZ();

    BinIDValueSet::Pos pos;
    while ( bivs.next(pos) )
    {
	BinID bid = bivs.getBinID(pos);
	float* vals = bivs.getVals(pos);
	const int inlidx = impbody_->cs_.hrg.inlRange().nearestIndex(bid.inl);
	const int crlidx = impbody_->cs_.hrg.crlRange().nearestIndex(bid.crl);
	if ( inlidx<0 || inlidx>=inlsz || crlidx<0 || crlidx>=crlsz )
	{
	    vals[valcol] = 0;
	    continue;
	}

	bool found = false;
	float minz=0, maxz=0;
	for ( int idz=0; idz<zsz; idz++ )
	{
	    if ( impbody_->arr_->get(inlidx,crlidx,idz)>impbody_->threshold_ )
		continue;

	    const float curz = impbody_->cs_.zrg.atIndex(idz);
	    if ( !found )
	    {
		found = true;
		minz = maxz = curz;
	    }
	    else
	    {
		if ( minz>curz ) 
		    minz = curz;
		else if ( maxz<curz ) 
		    maxz = curz;
	    }
	}
	    
	vals[valcol] = maxz-minz;
    }

    setRandomPosData( attrib, data, 0 );

    BufferString seqnm;
    Settings::common().get( "dTect.Color table.Horizon", seqnm );
    ColTab::Sequence seq( seqnm );
    setColTabSequence( attrib, seq, 0 );
    DPM( DataPackMgr::PointID() ).release( data->id() );
}


void MarchingCubesDisplay::setDepthAsAttrib( int attrib )
{
    mSetDataPointSet("Depth");
    BinIDValueSet::Pos pos;
    while ( bivs.next(pos) )
    {
	float* vals = bivs.getVals(pos);
	vals[valcol] = vals[0];
    }

    setRandomPosData( attrib, data, 0 );

    BufferString seqnm;
    Settings::common().get( "dTect.Color table.Horizon", seqnm );
    ColTab::Sequence seq( seqnm );
    setColTabSequence( attrib, seq, 0 );

    DPM( DataPackMgr::PointID() ).release( data->id() );
}


void MarchingCubesDisplay::getRandomPos( DataPointSet& dps,
					 TaskRunner* tr ) const
{
    if ( displaysurface_ )
	displaysurface_->getShape()->getAttribPositions( dps, tr );
}


void MarchingCubesDisplay::setRandomPosData( int attrib,
				 const DataPointSet* dps, TaskRunner* tr )
{
    if ( attrib<0 )
	return;

    DataPointSet* ndps = dps ? new DataPointSet( *dps ) : 0;
    if ( !attrib && dps && displaysurface_ )
    {
	displaysurface_->getShape()->setAttribData( *ndps, tr );
	useTexture( true );
	materialChangeCB( 0 );
    }

    if ( cache_.validIdx(attrib) )
    {
    	if ( cache_[attrib] )
	{
    	    DPM( DataPackMgr::PointID() ).release( cache_[attrib]->id() );
	    delete cache_[attrib];
	}

	cache_.replace(attrib,ndps);
    }
    else
    {
	while ( attrib>cache_.size() )
	    cache_ += 0;
	cache_ += ndps;
    }
    
    if ( cache_[attrib] )
	DPM( DataPackMgr::PointID() ).obtain( cache_[attrib]->id() );
}


void MarchingCubesDisplay::getMousePosInfo( const visBase::EventInfo& ei,
					    IOPar& iop ) const
{ SurveyObject::getMousePosInfo(ei,iop); }


void MarchingCubesDisplay::getMousePosInfo(const visBase::EventInfo&,
 			    Coord3& xyzpos, BufferString& val,
 			    BufferString& info) const
{
    val = sKey::EmptyString();
    info = "Body: ";
    info += name();

    int valididx = -1;
    for ( int idx=0; idx<cache_.size(); idx++ )
    {
    	if ( !cache_[idx] ) continue;
	valididx = idx;
	break;
    }

    if ( valididx==-1 )
	return;

    const BinIDValueSet& bivset = cache_[valididx]->bivSet();
    const BinID bid = SI().transform( xyzpos );

    TypeSet<float> zdist;
    TypeSet<float> vals;

    BinIDValueSet::Pos pos = bivset.findFirst( bid );
    const int validx = bivset.nrVals()-1;

    while ( pos.valid() )
    {
	const float* posvals = bivset.getVals(pos);
	const float depth = posvals[0];
	if ( !mIsUdf(depth) )
	{
	    zdist += (float) fabs(depth-xyzpos.z);
	    vals += posvals[validx];
	}

	if ( !bivset.next( pos, false ) || bivset.getBinID(pos)!=bid )
	    break;
    }

    if ( !zdist.size() )
	return;

    sort_coupled( zdist.arr(), vals.arr(), zdist.size() );

    if ( zdist[0]>SI().zRange(true).step )
	return;

    val = vals[0];
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool MarchingCubesDisplay::setEMID( const EM::ObjectID& emid,
       TaskRunner* tr )
{
    if ( emsurface_ )
	emsurface_->unRef();

    emsurface_ = 0;
    delete impbody_; impbody_ = 0;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::MarchingCubesSurface*, emmcsurf, emobject.ptr() );
    if ( !emmcsurf )
    {
	if ( displaysurface_ ) displaysurface_->turnOn( false );
	return false;
    }

    emsurface_ = emmcsurf;
    emsurface_->ref();

    updateVisFromEM( false, tr );
    return true;
}


void MarchingCubesDisplay::updateVisFromEM( bool onlyshape, TaskRunner* tr )
{
    if ( !onlyshape || !displaysurface_ )
    {
	getMaterial()->setColor( emsurface_->preferredColor() );
	if ( !emsurface_->name().isEmpty() )
	    setName( emsurface_->name() );
	else setName( "<New body>" );

	if ( !displaysurface_ )
	{
	    displaysurface_ = visBase::MarchingCubesSurface::create();
	    displaysurface_->ref();
	    displaysurface_->setMaterial( 0 );
	    displaysurface_->setSelectable( false );
	    displaysurface_->setRightHandSystem( righthandsystem_ );
	    addChild( displaysurface_->getInventorNode() );
	    materialChangeCB( 0 );
	}

	displaysurface_->setScales(
		SamplingData<float>(emsurface_->inlSampling()),
		SamplingData<float>(emsurface_->crlSampling()),
				    emsurface_->zSampling() );

	displaysurface_->setSurface( emsurface_->surface(), tr );
	displaysurface_->turnOn( true );
    }

    displaysurface_->touch( !onlyshape, tr );
}


MultiID MarchingCubesDisplay::getMultiID() const
{
    return emsurface_ ? emsurface_->multiID() : MultiID();
}


void MarchingCubesDisplay::setColor( Color nc )
{
    if ( emsurface_ ) emsurface_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


NotifierAccess* MarchingCubesDisplay::materialChange()
{ return &getMaterial()->change; }


Color MarchingCubesDisplay::getColor() const
{ return getMaterial()->getColor(); }


void MarchingCubesDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    par.set( sKeyEarthModelID(), getMultiID() );

    IOPar attribpar;
    selspec_.fillPar( attribpar ); //Right now only one attribute for the body

    if ( canSetColTabSequence() && getColTabSequence( 0 ) )
    {
	IOPar seqpar;
	const ColTab::Sequence* seq = getColTabSequence( 0 );
	if ( seq->isSys() )
	    seqpar.set( sKey::Name(), seq->name() );
	else
	    seq->fillPar( seqpar );
	
	attribpar.mergeComp( seqpar, sKeyColTabSequence() );
    }
    
    if ( getColTabMapperSetup( 0, 0 ) )
    {
	IOPar mapperpar;
	getColTabMapperSetup( 0, 0 )->fillPar( mapperpar );
	attribpar.mergeComp( mapperpar, sKeyColTabMapper() );
    }

    par.mergeComp( attribpar, sKeyAttribSelSpec() );
}


int MarchingCubesDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id(), 0 );
    }

    const IOPar* attribpar = par.subselect( sKeyAttribSelSpec() );
    if ( attribpar ) //Right now only one attribute for the body
    {
	selspec_.usePar( *attribpar );
	
	PtrMan<IOPar> seqpar = attribpar->subselect( sKeyColTabSequence() );
	if ( seqpar )
	{
	    ColTab::Sequence seq;
	    if ( !seq.usePar( *seqpar ) )
	    {
		BufferString seqname;
		if ( seqpar->get( sKey::Name(), seqname ) ) 
		    ColTab::SM().get( seqname.buf(), seq );
	    }
	    
	    setColTabSequence( 0, seq, 0 );
	}
	
	PtrMan<IOPar> mappar = attribpar->subselect( sKeyColTabMapper() );
 	if ( mappar )
	{
	    ColTab::MapperSetup mapper;
	    mapper.usePar( *mappar );
	    setColTabMapperSetup( 0, mapper, 0 );
	}
    }

    return 1;
}


void MarchingCubesDisplay::setDisplayTransformation( const mVisTrans* nt)
{
    if ( displaysurface_ ) displaysurface_->setDisplayTransformation( nt );

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
	intsinfo_[idx]->visshape_->setDisplayTransformation( nt );
}


void MarchingCubesDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaysurface_ ) displaysurface_->setRightHandSystem( yn );

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
	intsinfo_[idx]->visshape_->setRightHandSystem( yn );
}


const mVisTrans* MarchingCubesDisplay::getDisplayTransformation() const
{
    return displaysurface_ ? displaysurface_->getDisplayTransformation() : 0; 
}


void MarchingCubesDisplay::materialChangeCB( CallBacker* )
{
    if ( displaysurface_ )
	displaysurface_->getShape()->updateMaterialFrom( getMaterial() );

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
	intsinfo_[idx]->visshape_->setMaterial( getMaterial() );
}


void MarchingCubesDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    return; //TODO
    if ( !selector.isOK() || !displaysurface_ )
	return;

    const SamplingData<int>&  isp = emsurface_->inlSampling();
    const SamplingData<int>&  csp = emsurface_->crlSampling();
    const SamplingData<float>& zsp = emsurface_->zSampling();

    ::MarchingCubesSurface* mcs = displaysurface_->getSurface();
    
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcs || !mcs->models_.getRange(0,inlrg) || 
	 !mcs->models_.getRange(1,crlrg) || !mcs->models_.getRange(2,zrg) )
   	return;	

    const int inlsz = inlrg.width()+1;
    const int crlsz = crlrg.width()+1;
    const int zsz = zrg.width()+1;

#define mRetDetele() { delete arr; delete narr; return; }

    mDeclareAndTryAlloc( Array3DImpl<int>*, arr,
	    Array3DImpl<int>(inlsz,crlsz,zsz) );
    mDeclareAndTryAlloc( Array3DImpl<float>*, narr,
	    Array3DImpl<float>(inlsz,crlsz,zsz) );
    if ( !arr || !narr )
       mRetDetele()	

    MarchingCubes2Implicit m2i( *mcs, *arr, 0, 0, 0, false );
    if ( !m2i.execute() ) 
	mRetDetele()

    for ( int idx=0; idx<inlsz; idx++ )
    {
	const int inl = inlrg.start+idx*isp.step;
	for ( int idy=0; idy<crlsz; idy++ )
	{
	    const int crl = crlrg.start+idy*csp.step;
	    Coord3 pos( SI().transform(BinID(inl,crl)), 0 );
	    for ( int idz=0; idz<zsz; idz++ )
	    {
		const int val = arr->get(idx, idy, idz);
		narr->set(idx, idy, idz, val);
		if ( val>0 )
		    continue;

		pos.z = zrg.start+idz*zsp.step;
		if ( !selector.includes(pos) )
	    	    continue;

		arr->set( idx, idy, idz, 1 );
		narr->set(idx, idy, idz, 1);
	    }
	}
    }
    
    Implicit2MarchingCubes i2m(0,0,0,*narr,0,*mcs);
    if ( !i2m.execute() )
	mRetDetele()

    emsurface_->setChangedFlag();
}


void MarchingCubesDisplay::otherObjectsMoved( 
	const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    if ( !emsurface_ || !displaysurface_ )
	return;
    
    ObjectSet<const PlaneDataDisplay> activeplanes;
    TypeSet<int> activepids;
    
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	if ( !plane || !plane->isOn() )
	    continue;

	activeplanes += plane;
	activepids += plane->id();
    }

    for ( int idx=intsinfo_.size()-1; idx>=0; idx-- )
    {
	const int ipid = intsinfo_[idx]->planeid_;
	if ( (whichobj>=0 && ipid!=whichobj) || 
	     (whichobj<0 && activepids.isPresent(ipid)) )
	    continue;

	removeChild( intsinfo_[idx]->visshape_->getInventorNode() );	
	delete intsinfo_.remove( idx );
    }

    for ( int idx=0; idx<activeplanes.size(); idx++ )
    {
	bool planepresent = false;
	for ( int idy=0; idy<intsinfo_.size(); idy++ )
	{
    	    if ( intsinfo_[idy]->planeid_ == activepids[idx] )
	    {
		planepresent = true;
		break;
	    }
	}

	if ( planepresent ) continue;

	PlaneIntersectInfo* pi = new PlaneIntersectInfo();
	pi->visshape_->setDisplayTransformation( getDisplayTransformation() );
	pi->visshape_->setRightHandSystem( righthandsystem_ );
	pi->visshape_->setMaterial( getMaterial() );
	pi->visshape_->turnOn( displayintersections_ );
	addChild( pi->visshape_->getInventorNode() );

	CubeSampling cs = activeplanes[idx]->getCubeSampling(true,true,-1);
	PlaneDataDisplay::Orientation ori = activeplanes[idx]->getOrientation();
	const float pos = ori==PlaneDataDisplay::Zslice ? cs.zrg.start :
	    ori==PlaneDataDisplay::Inline ? cs.hrg.start.inl : cs.hrg.start.crl;

	pi->planeorientation_ = (char)ori;
	pi->planepos_ = pos;
	pi->planeid_ = activepids[idx];

	intsinfo_ += pi;
    }

    updateIntersectionDisplay();
}


bool MarchingCubesDisplay::areIntersectionsDisplayed() const
{ return displayintersections_; }


void MarchingCubesDisplay::displayIntersections( bool yn )
{
    if ( displayintersections_==yn )
	return;

    displayintersections_ = yn;
    updateIntersectionDisplay();
}


void MarchingCubesDisplay::updateIntersectionDisplay()
{
    if ( displayintersections_ )
    {
    	if ( !impbody_ )
    	    impbody_ = emsurface_->createImplicitBody(0,false);
	if ( !impbody_ ) return;
	
    	for ( int idx=0; idx<intsinfo_.size(); idx++ )
    	{
    	    if ( intsinfo_[idx]->computed_ )
    		continue;
	    
    	    intsinfo_[idx]->computed_ = true;
    	    Geometry::ImplicitBodyPlaneIntersector gii( *impbody_->arr_, 
    		    impbody_->cs_, impbody_->threshold_, 
    		    intsinfo_[idx]->planeorientation_, 
		    intsinfo_[idx]->planepos_, *intsinfo_[idx]->shape_ );
    	    gii.compute();
    	}
    }

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
    {
	if ( displayintersections_ )
	    intsinfo_[idx]->visshape_->touch( false );
	
	intsinfo_[idx]->visshape_->turnOn( displayintersections_ );
    }
	
    if ( displaysurface_ ) 
	displaysurface_->turnOn( !displayintersections_ );
}


MarchingCubesDisplay::PlaneIntersectInfo::PlaneIntersectInfo()
{
    planeid_ = -1;
    planeorientation_ = -1;
    planepos_ = mUdf(float);
    computed_ = false;

    visshape_ = visBase::GeomIndexedShape::create();
    visshape_->turnOnForegroundLifter( true );
    if ( visshape_->getMaterial() )
	visshape_->setMaterial( visBase::Material::create() );
    visshape_->ref();
    visshape_->setSelectable( false );
    visshape_->renderOneSide( 0 );
    
    shape_ = new Geometry::ExplicitIndexedShape();
    visshape_->setSurface( shape_ );
    shape_->addGeometry( new Geometry::IndexedGeometry(
		Geometry::IndexedGeometry::TriangleStrip,
		Geometry::IndexedGeometry::PerVertex, shape_->coordList(),
		shape_->normalCoordList(),shape_->textureCoordList()) );
}



MarchingCubesDisplay::PlaneIntersectInfo::~PlaneIntersectInfo()
{
    visshape_->unRef();
    delete shape_;
}


}; // namespace visSurvey
