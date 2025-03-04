/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismarchingcubessurfacedisplay.h"

#include "color.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emmanager.h"
#include "executor.h"
#include "indexedshape.h"
#include "impbodyplaneintersect.h"
#include "keystrs.h"
#include "marchingcubes.h"
#include "positionlist.h"
#include "posvecdataset.h"
#include "randcolor.h"
#include "selector.h"
#include "settings.h"
#include "survinfo.h"
#include "uistrings.h"

#include "visplanedatadisplay.h"
#include "vismaterial.h"
#include "vispolygonoffset.h"


namespace visSurvey
{

MarchingCubesDisplay::MarchingCubesDisplay()
    : visBase::VisualObjectImpl(true)
    , selspecs_(1,Attrib::SelSpec())
{
    ref();
    datapacks_.setNullAllowed();
    setColor( OD::getRandomColor(false) );
    getMaterial()->setAmbience( 0.4 );
    mAttachCB( getMaterial()->change, MarchingCubesDisplay::materialChangeCB );
    unRefNoDelete();
}


MarchingCubesDisplay::~MarchingCubesDisplay()
{
    detachAllNotifiers();
    delete impbody_;
    deepErase( intsinfo_ );
}


void MarchingCubesDisplay::useTexture( bool yn, bool trigger )
{
    usestexture_ = displaysurface_ && yn;
    updateSingleColor();
}


bool MarchingCubesDisplay::usesColor() const
{
    return !showsTexture() || displayedOnlyAtSections();
}


void MarchingCubesDisplay::updateSingleColor()
{
    if ( displaysurface_ )
	displaysurface_->getShape()->enableColTab( showsTexture() );
}


bool MarchingCubesDisplay::canShowTexture() const
{
    return validtexture_ && isAnyAttribEnabled() &&
	   !displayedOnlyAtSections();
}


bool MarchingCubesDisplay::setVisSurface(visBase::MarchingCubesSurface* surface)
{
    if ( displaysurface_ )
    {
	removeChild( displaysurface_->osgNode() );
	displaysurface_ = nullptr;
    }

    emsurface_ = nullptr;
    deleteAndNullPtr( impbody_ );
    if ( !surface || !surface->getSurface() )
	return false;

    emsurface_ = new EM::MarchingCubesSurface( EM::EMM() );
    if ( !emsurface_ )
    {
	for ( auto* intsinfo : intsinfo_ )
	    intsinfo->visshape_->turnOn( false );

	return false;
    }

    if ( !emsurface_->isOK() ||
	 !emsurface_->setSurface( surface->getSurface() ) )
    {
	emsurface_ = nullptr;
	return false;
    }

    SamplingData<float> sd = surface->getScale( 0 );
    emsurface_->setInlSampling(
                SamplingData<int>( mNINT32(sd.start_), mNINT32(sd.step_) ) );

    sd = surface->getScale( 1 );
    emsurface_->setCrlSampling(
                SamplingData<int>( mNINT32(sd.start_), mNINT32(sd.step_) ) );

    emsurface_->setZSampling( surface->getScale( 2 ) );

    EM::EMM().addObject( emsurface_.ptr() );

    displaysurface_ = surface;
    displaysurface_->setSelectable( false );
    addChild( displaysurface_->osgNode() );

    if ( displaysurface_->getMaterial() )
	getMaterial()->setFrom( *displaysurface_->getMaterial() );

    displaysurface_->setMaterial( nullptr );
    mDetachCB( getMaterial()->change, MarchingCubesDisplay::materialChangeCB );
    emsurface_->setPreferredColor( getColor() );
    emsurface_->setName( name() );

    materialChangeCB( nullptr );
    return true;
}


EM::ObjectID MarchingCubesDisplay::getEMID() const
{
    return emsurface_ ? emsurface_->id() : EM::ObjectID::udf();
}


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
		? displaysurface_->getShape()->getDataMapper() : nullptr;
}


void MarchingCubesDisplay::setColTabMapperSetup( int attrib,
	const ColTab::MapperSetup& setup, TaskRunner* taskrunner )
{
    if ( !attrib && displaysurface_ )
	displaysurface_->getShape()->setDataMapper( setup, taskrunner );
}


const ColTab::Sequence*
MarchingCubesDisplay::getColTabSequence( int attrib ) const
{
    return !attrib && displaysurface_
		? displaysurface_->getShape()->getDataSequence() : nullptr;
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
    SurveyObject::setSelSpec( attrib, spec );
    if ( !attrib )
	selspecs_.first() = spec;
}


void MarchingCubesDisplay::setSelSpecs(
		int attrib, const TypeSet<Attrib::SelSpec>& spec )
{
    SurveyObject::setSelSpecs( attrib, spec );
    if ( !attrib )
	selspecs_.first() = spec.first();
}


const Attrib::SelSpec* MarchingCubesDisplay::getSelSpec(
					int attrib, int /* version */ ) const
{
    return attrib ? nullptr : &(selspecs_.first());
}


const TypeSet<Attrib::SelSpec>* MarchingCubesDisplay::getSelSpecs(
							int attrib ) const
{
    return attrib ? nullptr : &selspecs_;
}


#define mSetDataPointSet(nm) \
    const bool attribselchange = \
		StringView(selspecs_.first().userRef())!=nm; \
    selspecs_.first().set( nm, Attrib::SelSpec::cNoAttrib(), false, "" ); \
    RefMan<DataPointSet> dps = new DataPointSet(false,true); \
    getRandomPos( *dps.ptr(), nullptr ); \
    dps->setName( nm ); \
    auto* isovdef = new DataColDef(nm); \
    dps->dataSet().add( isovdef ); \
    BinIDValueSet& bivs = dps->bivSet();  \
    if ( dps->isEmpty() || bivs.nrVals()!=3 ) \
	return; \
    int valcol = dps->dataSet().findColDef( *isovdef, \
					    PosVecDataSet::NameExact ); \
    if ( valcol==-1 ) valcol = 1


void MarchingCubesDisplay::setIsopach( int attrib )
{
    mSetDataPointSet("Isopach");

    if ( !impbody_ && emsurface_ )
	impbody_ = emsurface_->createImplicitBody( 0, false );

    if ( !impbody_ || !impbody_->arr_ )
	return;

    const int inlsz = impbody_->tkzs_.nrInl();
    const int crlsz = impbody_->tkzs_.nrCrl();
    const int zsz = impbody_->tkzs_.nrZ();

    BinIDValueSet::SPos pos;
    while ( bivs.next(pos) )
    {
	BinID bid = bivs.getBinID(pos);
	float* vals = bivs.getVals(pos);

	const int inlidx =
	    impbody_->tkzs_.hsamp_.inlRange().nearestIndex(bid.inl());
	const int crlidx =
	    impbody_->tkzs_.hsamp_.crlRange().nearestIndex(bid.crl());

	if ( inlidx<0 || inlidx>=inlsz || crlidx<0 || crlidx>=crlsz )
	{
	    vals[valcol] = 0;
	    continue;
	}

	bool found = false;
	float minz=0, maxz=0;
	for ( int idz=0; idz<zsz; idz++ )
	{
	    if ( impbody_->arr_->get(inlidx,crlidx,idz) < impbody_->threshold_ )
		continue;

	    const float curz = impbody_->tkzs_.zsamp_.atIndex(idz);
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

    setRandomPosData( attrib, dps.ptr(), nullptr );

    if ( attribselchange )
    {
	BufferString seqnm;
	Settings::common().get( "dTect.Horizon.Color table", seqnm );
	ColTab::Sequence seq( seqnm );
	setColTabSequence( attrib, seq, nullptr );
	setColTabMapperSetup( attrib, ColTab::MapperSetup(), nullptr );
    }
}


void MarchingCubesDisplay::setDepthAsAttrib( int attrib )
{
    mSetDataPointSet("Z values");
    BinIDValueSet::SPos pos;
    while ( bivs.next(pos) )
    {
	float* vals = bivs.getVals(pos);
	vals[valcol] = vals[0];
    }

    setRandomPosData( attrib, dps.ptr(), nullptr );

    if ( attribselchange )
    {
	BufferString seqnm;
	Settings::common().get( "dTect.Horizon.Color table", seqnm );
	ColTab::Sequence seq( seqnm );
	setColTabSequence( attrib, seq, nullptr );
	setColTabMapperSetup( attrib, ColTab::MapperSetup(), nullptr );
    }
}


bool MarchingCubesDisplay::getRandomPos( DataPointSet& dpset,
					 TaskRunner* runner ) const
{
    for ( int idx=0; idx<datapacks_.size(); idx++ )
    {
	ConstRefMan<DataPointSet> dps = datapacks_[idx];
	if ( !dps || dps->size() < 1 )
	    continue;

	dpset = *dps.ptr();
	dpset.dataSet().removeColumn( dps->dataSet().nrCols()-1 );
	return true;
    }

    if ( !displaysurface_ || !emsurface_ )
	return false;

    SamplingData<float> inlinesampling = emsurface_->inlSampling();
    SamplingData<float> crlinesampling = emsurface_->crlSampling();
    SamplingData<float> zsampling = emsurface_->zSampling();

    RefMan<visBase::Transformation> toincrltransf =
					visBase::Transformation::create();
    toincrltransf->setScale( Coord3(inlinesampling.step_,
				    crlinesampling.step_, zsampling.step_));
    toincrltransf->setTranslation( Coord3(inlinesampling.start_,
				crlinesampling.start_, zsampling.start_));

    return displaysurface_->getShape()->getAttribPositions( dpset,
						toincrltransf.ptr(), runner );
}


bool MarchingCubesDisplay::setPointDataPack( int attrib, PointDataPack* dp,
					     TaskRunner* runner )
{
    mDynamicCastGet(DataPointSet*,dps,dp);
    if ( !dps || attrib<0 )
	return false;

    if ( !attrib && dps && displaysurface_ )
    {
	displaysurface_->getShape()->setAttribData( *dps, runner );
	materialChangeCB( nullptr );
    }

    if ( datapacks_.validIdx(attrib) )
	datapacks_.replace( attrib, dps );
    else
    {
	while ( attrib>datapacks_.size() )
	    datapacks_ += nullptr;

	datapacks_ += (DataPointSet*) dps;
    }

    validtexture_ = true;
    updateSingleColor();
    return true;
}


bool MarchingCubesDisplay::setRandomPosData( int attrib,
					     const DataPointSet* dps,
					     TaskRunner* runner )
{
    return setPointDataPack( attrib, const_cast<DataPointSet*>( dps ), runner );
}


ConstRefMan<DataPack> MarchingCubesDisplay::getDataPack( int attrib ) const
{
    return getPointDataPack( attrib );
}


ConstRefMan<PointDataPack>
		MarchingCubesDisplay::getPointDataPack( int attrib ) const
{
    if ( !datapacks_.validIdx(attrib) )
	return nullptr;

    ConstRefMan<DataPointSet> datapack = datapacks_.get( attrib );
    return datapack ? datapack.ptr() : nullptr;
}


void MarchingCubesDisplay::getMousePosInfo( const visBase::EventInfo& ei,
					    IOPar& iop ) const
{
    SurveyObject::getMousePosInfo(ei,iop);
}


void MarchingCubesDisplay::getMousePosInfo(const visBase::EventInfo&,
			    Coord3& xyzpos, BufferString& val,
			    uiString& info) const
{
    val.setEmpty();
    info.set( uiStrings::sGeobody() ).addMoreInfo( name() );

    int valididx = -1;
    for ( int idx=0; idx<datapacks_.size(); idx++ )
    {
	if ( !datapacks_[idx] )
	    continue;

	valididx = idx;
	break;
    }

    if ( valididx==-1 )
	return;

    const BinIDValueSet& bivset = datapacks_[valididx]->bivSet();
    const BinID bid = SI().transform( xyzpos );

    TypeSet<float> zdist;
    TypeSet<float> vals;

    BinIDValueSet::SPos pos = bivset.find( bid );
    const int validx = bivset.nrVals()-1;

    while ( pos.isValid() )
    {
	const float* posvals = bivset.getVals(pos);
	const float depth = posvals[0];
	if ( !mIsUdf(depth) )
	{
            zdist += (float) fabs(depth-xyzpos.z_);
	    vals += posvals[validx];
	}

	if ( !bivset.next( pos, false ) || bivset.getBinID(pos)!=bid )
	    break;
    }

    if ( !zdist.size() )
	return;

    sort_coupled( zdist.arr(), vals.arr(), zdist.size() );

    if ( zdist[0]>SI().zRange(true).step_ )
	return;

    val = vals[0];
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool MarchingCubesDisplay::setEMID( const EM::ObjectID& emid,
				    TaskRunner* runner )
{
    emsurface_ = nullptr;
    deleteAndNullPtr( impbody_ );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::MarchingCubesSurface*, emmcsurf, emobject.ptr() );
    if ( !emmcsurf )
    {
	if ( displaysurface_ )
	    displaysurface_->turnOn( false );

	return false;
    }

    emsurface_ = emmcsurf;

    return updateVisFromEM( false, runner );
}


bool MarchingCubesDisplay::updateVisFromEM( bool onlyshape, TaskRunner* runner )
{
    if ( emsurface_ && (!onlyshape || !displaysurface_) )
    {
	getMaterial()->setColor( emsurface_->preferredColor() );
	if ( emsurface_->name().isEmpty() )
	    setName( "<New Geobody>" );
	else
	    setName( emsurface_->name() );

	if ( !displaysurface_ )
	{
	    displaysurface_ = visBase::MarchingCubesSurface::create();
	    displaysurface_->setMaterial( nullptr );
	    displaysurface_->setSelectable( false );
	    displaysurface_->setRightHandSystem( righthandsystem_ );
	    addChild( displaysurface_->osgNode() );
	    materialChangeCB( nullptr );
	}

	displaysurface_->setScales(
			    SamplingData<float>(emsurface_->inlSampling()),
			    SamplingData<float>(emsurface_->crlSampling()),
			    emsurface_->zSampling() );

	if ( !displaysurface_->setSurface(emsurface_->surface(),runner) )
	{
	    removeChild( displaysurface_->osgNode() );
	    displaysurface_ = nullptr;
	    return false;
	}
	else
	    displaysurface_->turnOn( true );
    }

    return displaysurface_->touch( !onlyshape, runner );
}


MultiID MarchingCubesDisplay::getMultiID() const
{
    return emsurface_ ? emsurface_->multiID() : MultiID();
}


void MarchingCubesDisplay::setColor( OD::Color nc )
{
    if ( emsurface_ )
	emsurface_->setPreferredColor(nc);

    getMaterial()->setColor( nc );
}


OD::Color MarchingCubesDisplay::getColor() const
{
    return getMaterial()->getColor();
}


void MarchingCubesDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );
    par.set( sKeyEarthModelID(), getMultiID() );
    par.setYN( sKeyUseTexture(), usestexture_ );

    IOPar attribpar;
	selspecs_.first().fillPar( attribpar );
	//Right now only one attribute for the body

    if ( canSetColTabSequence() && getColTabSequence(0) )
    {
	IOPar seqpar;
	const ColTab::Sequence* seq = getColTabSequence( 0 );
	if ( seq->isSys() )
	    seqpar.set( sKey::Name(), seq->name() );
	else
	    seq->fillPar( seqpar );

	attribpar.mergeComp( seqpar, sKeyColTabSequence() );
    }

    if ( getColTabMapperSetup(0,0) )
    {
	IOPar mapperpar;
	getColTabMapperSetup(0,0)->fillPar( mapperpar );
	attribpar.mergeComp( mapperpar, sKeyColTabMapper() );
    }

    par.mergeComp( attribpar, sKeyAttribSelSpec() );
}


bool MarchingCubesDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar(par) || !SurveyObject::usePar(par) )
	 return false;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader )
		loader->execute();

	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject )
	    setEMID( emobject->id(), nullptr );
    }

    par.getYN( sKeyUseTexture(), usestexture_ );
    updateSingleColor();

    const IOPar* attribpar = par.subselect( sKeyAttribSelSpec() );
    if ( attribpar ) //Right now only one attribute for the body
    {
	selspecs_.first().usePar( *attribpar );

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

	    setColTabSequence( 0, seq, nullptr );
	}

	PtrMan<IOPar> mappar = attribpar->subselect( sKeyColTabMapper() );
	if ( mappar )
	{
	    ColTab::MapperSetup mapper;
	    mapper.usePar( *mappar );
	    setColTabMapperSetup( 0, mapper, nullptr );
	}
    }

    return true;
}


void MarchingCubesDisplay::setDisplayTransformation( const mVisTrans* nt)
{
    intersectiontransform_ = nt;
    if ( emsurface_ )
    {
	SamplingData<float> inlinesampling = emsurface_->inlSampling();
	SamplingData<float> crlinesampling = emsurface_->crlSampling();
	SamplingData<float> zsampling = emsurface_->zSampling();

	model2displayspacetransform_ = visBase::Transformation::create();
	model2displayspacetransform_->setScale(
		    Coord3( inlinesampling.step_, crlinesampling.step_,
			    zsampling.step_));
	model2displayspacetransform_->setTranslation(
		    Coord3( inlinesampling.start_, crlinesampling.start_,
			    zsampling.start_) );

	*model2displayspacetransform_ *= *nt;
    }

    if ( displaysurface_ )
	displaysurface_->setDisplayTransformation(
					model2displayspacetransform_.ptr());

}


const mVisTrans* MarchingCubesDisplay::getDisplayTransformation() const
{
    return displaysurface_ ? displaysurface_->getDisplayTransformation()
			   : nullptr;
}


void MarchingCubesDisplay::materialChangeCB( CallBacker* )
{
    if ( displaysurface_ )
	displaysurface_->getShape()->updateMaterialFrom( getMaterial() );

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
	intsinfo_[idx]->visshape_->updateMaterialFrom( getMaterial() );
}


void MarchingCubesDisplay::removeSelection( const Selector<Coord3>& selector,
					    TaskRunner* runner )
{
    return; //TODO
    /*
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
    */
}


void MarchingCubesDisplay::otherObjectsMoved(
				const ObjectSet<const SurveyObject>& objs,
				const VisID& whichobj )
{
    if ( !emsurface_ || !displaysurface_ )
	return;

    ObjectSet<const PlaneDataDisplay> activeplanes;
    TypeSet<VisID> activepids;
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
	const VisID ipid = intsinfo_[idx]->planeid_;
	if ( (whichobj.isValid() && ipid!=whichobj) ||
	     (!whichobj.isValid() && activepids.isPresent(ipid)) )
	    continue;

	removeChild( intsinfo_[idx]->visshape_->osgNode() );
	delete intsinfo_.removeSingle( idx );
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

	auto* pi = new PlaneIntersectInfo();
	pi->visshape_->updateMaterialFrom( getMaterial() );
	pi->visshape_->turnOn( displayintersections_ );
	addChild( pi->visshape_->osgNode() );

	TrcKeyZSampling cs =
	    activeplanes[idx]->getTrcKeyZSampling(true,true,-1);

	OD::SliceType ori = activeplanes[idx]->getOrientation();
	const float pos = ori==OD::SliceType::Z
                          ? cs.zsamp_.start_
	    : (ori==OD::SliceType::Inline
		? cs.hsamp_.start_.inl()
		: cs.hsamp_.start_.crl());

	pi->planeorientation_ = (char)ori;
	pi->planepos_ = pos;
	pi->planeid_ = activepids[idx];

	intsinfo_ += pi;
    }

    updateIntersectionDisplay();
}


bool MarchingCubesDisplay::displayedOnlyAtSections() const
{ return displayintersections_; }


void MarchingCubesDisplay::setOnlyAtSectionsDisplay( bool yn )
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
	if ( !impbody_ && emsurface_ )
	    impbody_ = emsurface_->createImplicitBody( 0, false );

	if ( !impbody_ )
	    return;

	for ( int idx=0; idx<intsinfo_.size(); idx++ )
	{
	    if ( intsinfo_[idx]->computed_ )
		continue;

	    intsinfo_[idx]->computed_ = true;
	    Geometry::ImplicitBodyPlaneIntersector gii( *impbody_->arr_,
		    impbody_->tkzs_, impbody_->threshold_,
		    intsinfo_[idx]->planeorientation_,
		    intsinfo_[idx]->planepos_, *intsinfo_[idx]->shape_ );
	    gii.compute();
	}
    }

    for ( int idx=0; idx<intsinfo_.size(); idx++ )
    {
	if ( displayintersections_ )
	{
	    intsinfo_[idx]->visshape_->setDisplayTransformation(
						intersectiontransform_.ptr() );
	    intsinfo_[idx]->visshape_->touch( false, false );
	}

	intsinfo_[idx]->visshape_->turnOn( displayintersections_ );
    }

    if ( displaysurface_ )
	displaysurface_->turnOn( !displayintersections_ );
}


void MarchingCubesDisplay::enableAttrib( int attrib, bool yn )
{
    if ( attrib != 0 )
	return;

    isattribenabled_ = yn;
    updateSingleColor();
}


bool MarchingCubesDisplay::isAttribEnabled( int attrib ) const
{
    return attrib==0 ? isattribenabled_ : false;
}


// MarchingCubesDisplay::PlaneIntersectInfo

MarchingCubesDisplay::PlaneIntersectInfo::PlaneIntersectInfo()
{
    planeid_.setUdf();
    RefMan<visBase::PolygonOffset> offset = visBase::PolygonOffset::create();
    offset->setFactor( -1.0f );
    offset->setUnits( 1.0f );
    offset->setMode(
	visBase::PolygonOffset::Protected | visBase::PolygonOffset::On  );

    visshape_ = visBase::GeomIndexedShape::create();
    visshape_->setSelectable( false );
    shape_ = new Geometry::ExplicitIndexedShape();

    visshape_->addNodeState( offset.ptr() );
    visshape_->setSurface( shape_ );
    visshape_->setGeometryShapeType( visBase::GeomIndexedShape::Triangle );
    visshape_->setNormalBindType( visBase::VertexShape::BIND_OVERALL );
    visshape_->setRenderMode( visBase::RenderBothSides );

   /* Coord3List* crdlist = shape_->normalCoordList();
    crdlist->add( Coord3( 1, 0, 0) );*/
    shape_->addGeometry( new Geometry::IndexedGeometry(
		Geometry::IndexedGeometry::TriangleStrip,shape_->coordList(),
		shape_->normalCoordList(),shape_->textureCoordList()) );
}


MarchingCubesDisplay::PlaneIntersectInfo::~PlaneIntersectInfo()
{
    delete shape_;
}

} // namespace visSurvey
