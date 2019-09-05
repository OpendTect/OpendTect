/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        D. Zheng
 Date:          Feb 2013
________________________________________________________________________

-*/

#include "vishorizondisplay.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "bidvsetarrayadapter.h"
#include "binidvalue.h"
#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "datapointset.h"
#include "datadistributionextracter.h"
#include "datacoldef.h"
#include "emhorizon3d.h"
#include "emobjectposselector.h"
#include "emsurfaceauxdata.h"
#include "horsubsel.h"
#include "isocontourtracer.h"
#include "odviscommon.h"
#include "survinfo.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "callback.h"

#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpe.h"
#include "vishorizontexturehandler.h"
#include "vishorizonsection.h"
#include "vishorizonsectiondef.h"
#include "visplanedatadisplay.h"
#include "vispointset.h"
#include "vispolyline.h"
#include "visrandomtrackdisplay.h"
#include "vistexturechannel2rgba.h"
#include "vistexturechannels.h"
#include "vismultiattribsurvobj.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "zaxistransform.h"



namespace visSurvey
{


class LockedPointsPathFinder: public ParallelTask
{
public:
			LockedPointsPathFinder(const EM::Horizon3D& hor,
					       const TrcKeyPath& path,
					       visBase::PointSet& points,
					       TypeSet<int>& indexes)
			    : path_(path)
			    , hor_(hor)
			    , points_(points)
			    , indexes_(indexes)
			    , lock_(Threads::Lock::SmallWork)
			{}

    od_int64		nrIterations() const	{ return path_.size(); }

protected:

    bool		doWork(od_int64 start,od_int64 stop,int thread);

    const EM::Horizon3D&	hor_;
    const TrcKeyPath&		path_;
    visBase::PointSet&		points_;
    TypeSet<int>&		indexes_;
    Threads::Lock		lock_;
};


bool LockedPointsPathFinder::doWork( od_int64 start, od_int64 stop, int thread )
{
    const Array2D<char>* lockednodes = hor_.getLockedNodes();
    if ( !lockednodes )
	return true;

    const TrcKeySampling tks = hor_.getTrackingSampling();

    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const TrcKey tk = path_[idx];
	if ( !tks.includes(tk) )
	    continue;

	const od_int64 gidx = tks.globalIdx( tk );
	if ( gidx>=0 && lockednodes->getData()[gidx] != '0' )
	{
	    const Coord3 pos =
		hor_.getPos( EM::PosID::getFromRowCol(tk.binID()) );
	    Threads::Locker locker( lock_, Threads::Locker::WriteLock );
	    indexes_ += points_.addPoint( pos );
	}
    }

    return true;
}

//==============================================================================

class HorizonPathIntersector : public ParallelTask
{
public:
		HorizonPathIntersector(const HorizonDisplay& hd,
				       const TrcKeyPath& path,
				       const TypeSet<Coord>& crds,
				       const Interval<float>& zrg,
				       HorizonDisplay::IntersectionData& res)
		    : hd_(hd), path_(path), crds_(crds)
		    , zrg_(zrg), res_(res)
		    , hor_(0), seedposids_(0), onthesamegrid_(false)
		    , pathgeom_(0), horgeom_(0)
		{
		    positions_ = new Coord3[ path_.size() ];
		}

		~HorizonPathIntersector()	{ delete [] positions_; }

    od_int64	nrIterations() const		{ return path_.size(); }

    bool	doPrepare(int nrthreads);
    bool	doWork(od_int64 start,od_int64 stop,int thread);
    bool	doFinish(bool success);

protected:

    static Coord3	skipPos()	{ return Coord3(Coord::udf(),0); }
    static Coord3	endLine()	{ return Coord3(Coord::udf(),1); }

    const HorizonDisplay&		hd_;
    const TrcKeyPath&			path_;
    const TypeSet<Coord>&		crds_;
    const Interval<float>&		zrg_;
    HorizonDisplay::IntersectionData&	res_;
    const EM::Horizon3D*		hor_;
    const TypeSet<EM::PosID>*		seedposids_;
    bool				onthesamegrid_;
    const SurvGeom*			pathgeom_;
    const SurvGeom*			horgeom_;

    Coord3*				positions_;
};


bool HorizonPathIntersector::doPrepare( int nrthreads )
{
    mDynamicCast( const EM::Horizon3D*, hor_, hd_.emobject_ );

    if ( !path_.size() )
	return false;

    seedposids_ = hor_->getPosAttribList ( EM::Object::sSeedNode() );

    const Pos::GeomID pathgeomid = path_[0].geomID();
    const Pos::GeomID horgeomid = hor_->getSurveyGeomID();
    pathgeom_ = &SurvGeom::get( pathgeomid );
    horgeom_ = &SurvGeom::get( horgeomid );
    onthesamegrid_ = pathgeomid==horgeomid;

    return pathgeom_ && horgeom_;
}


bool HorizonPathIntersector::doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	TrcKey hortrcky;
	const Coord intersectioncoord = idx>=crds_.size()
		? pathgeom_->toCoord( path_[idx] )
		: crds_[idx];

	if ( intersectioncoord.isDefined() )
	{
	    if ( onthesamegrid_ )
		hortrcky = path_[idx];
	    else
		horgeom_->getTracePosition( intersectioncoord, hortrcky,
					     horgeom_->averageTrcDist() );

	    if ( !hor_->range().includes(hortrcky) )
		{ positions_[idx] = skipPos(); continue; }

	    EM::PosID horsubid = hortrcky.isUdf()
		    ? EM::PosID::getInvalid()
		    : EM::PosID::getFromRowCol( hortrcky.binID() );

	    if ( hd_.showsPosAttrib(EM::PosAttrib::SeedNode) &&
		 seedposids_ && !horsubid.isInvalid() &&
		 seedposids_->isPresent(horsubid) )
	    {
		horsubid = EM::PosID::getInvalid();
	    }

	    if ( !horsubid.isInvalid() )
	    {
		//As zrg is in the non-transformed z-domain, we have to do the
		//comparison there.
		const Coord3 horpos = hor_->getPos( horsubid );
		Coord3 displayhorpos = horpos;
		if ( horpos.isDefined() && hd_.zaxistransform_ )
		{
		    displayhorpos.z_ =
			hd_.zaxistransform_->transformTrc( hortrcky,
							   (float) horpos.z_ );
		}

		if ( displayhorpos.isDefined() &&
		     zrg_.includes(horpos.z_,false) )
		{
		    //Take coord from intersection, and z from horizon
		    //Gives nice intersection when geometry is different
		    //such as on 2D lines
		    positions_[idx] = Coord3( intersectioncoord,
					      displayhorpos.z_ );
		    continue;
		}
	    }
	}

	positions_[idx] = endLine();
    }

    return true;
}


bool HorizonPathIntersector::doFinish( bool success )
{
    TypeSet<Coord3> curline;
    for ( int idx=0; idx<path_.size(); idx++ )
    {
	const Coord3 pos = positions_[idx];
	if ( pos == skipPos() )
	    continue;

	if ( pos != endLine() )
	    curline += pos;
	else if ( !curline.isEmpty() )
	{
	    res_.addLine( curline );
	    curline.erase();
	}
    }

    res_.addLine( curline );
    return success;
}

//==============================================================================

const char* HorizonDisplay::sKeyTexture()	{ return "Use texture"; }
const char* HorizonDisplay::sKeyShift()		{ return "Shift"; }
const char* HorizonDisplay::sKeyResolution()	{ return "Resolution"; }
const char* HorizonDisplay::sKeyRowRange()	{ return "Row range"; }
const char* HorizonDisplay::sKeyColRange()	{ return "Col range"; }
const char* HorizonDisplay::sKeySurfaceGrid()	{ return "SurfaceGrid"; }
const char* HorizonDisplay::sKeyIntersectLineMaterialID()
{ return "Intsectline material id"; }
const char* HorizonDisplay::sKeySectionID()	{ return "Section ID"; }
const char* HorizonDisplay::sKeyZValues()	{ return "Z values"; }


HorizonDisplay::HorizonDisplay()
    : parrowrg_( -1, -1, -1 )
    , parcolrg_( -1, -1, -1 )
    , curtextureidx_( 0 )
    , usestexture_( true )
    , translation_( 0 )
    , validtexture_( false )
    , resolution_( 0 )
    , allowshading_( true )
    , intersectionlinematerial_( 0 )
    , displayintersectionlines_( true )
    , enabletextureinterp_( true )
    , displaysurfacegrid_( false )
    , translationpos_( Coord3().udf() )
    , parentline_( 0 )
    , selections_( 0 )
    , lockedpts_( 0 )
    , sectionlockedpts_( 0 )
    , showlock_( false )
{
    translation_ = visBase::Transformation::create();
    translation_->ref();

    setGroupNode( (osg::Group*) translation_->osgNode() );

    setLockable();
    maxintersectionlinethickness_ = 0.02f *
	mMAX( s3dgeom_->inlDistance() * s3dgeom_->inlRange().width(),
	      s3dgeom_->crlDistance() * s3dgeom_->crlRange().width() );

    as_ += new Attrib::SelSpecList( 1, Attrib::SelSpec() );
    coltabmappers_ += new ColTab::Mapper;
    coltabsequences_ += ColTab::SeqMGR().getDefault();

    TypeSet<float> shift;
    shift += 0.0;
    curshiftidx_ += 0;
    BufferStringSet* attrnms = new BufferStringSet();
    attrnms->setNullAllowed();
    userrefs_ += attrnms;
    shifts_ += new TypeSet<float>;
    enabled_ += true;
    dispdatapackids_ += new TypeSet<DataPack::ID>;

    material_->setAmbience( 0.7 );

    RefMan<visBase::Material> linemat = new visBase::Material;
    linemat->setFrom( *material_ );
    linemat->setColor( nontexturecol_ );
    linemat->setDiffIntensity( 1 );
    linemat->setAmbience( 1 );
    setIntersectLineMaterial( linemat );

    resolution_ = (char)OD::getDefaultSurfaceResolution();
}


HorizonDisplay::~HorizonDisplay()
{
    deepErase(as_);
    unRefAndZeroPtr( intersectionlinematerial_ );

    setSceneEventCatcher( 0 );
    curshiftidx_.erase();

    if ( translation_ )
    {
	translation_->unRef();
	translation_ = 0;
    }

    removeEMStuff();
    unRefAndZeroPtr( zaxistransform_ );

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int idx=0; idx<dispdatapackids_.size(); idx++ )
    {
	const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[idx];
	for ( int idy=dpids.size()-1; idy>=0; idy-- )
	    dpm.unRef( dpids[idy] );
    }

    deepErase( dispdatapackids_ );
    deepErase( shifts_ );

    if ( selections_ ) selections_->unRef();
    if ( lockedpts_ ) lockedpts_->unRef();
    if ( sectionlockedpts_ ) sectionlockedpts_->unRef();
    emchangedata_.clearData();
}


void HorizonDisplay::displayIntersectionLines( bool yn )
{
    displayintersectionlines_ = yn;

    hasmoved.trigger();
    changedisplay.trigger();
}


bool HorizonDisplay::displaysIntersectionLines() const
{
    return displayintersectionlines_;
}


void HorizonDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<intersectiondata_.size(); idx++ )
	intersectiondata_[idx]->setDisplayTransformation(transformation_);

    if ( translationpos_.isDefined() )
	setTranslation( translationpos_ );

    if ( parentline_ )
	parentline_->setDisplayTransformation( transformation_ );
    if ( selections_ )
	selections_->setDisplayTransformation( transformation_ );
    if ( lockedpts_ )
	lockedpts_->setDisplayTransformation( transformation_ );
    if ( sectionlockedpts_ )
	sectionlockedpts_->setDisplayTransformation( transformation_ );
}


bool HorizonDisplay::setZAxisTransform( ZAxisTransform* nz, TaskRunner* tskr )
{
    if ( zaxistransform_ ) zaxistransform_->unRef();

    zaxistransform_ = nz;
    if ( zaxistransform_ ) zaxistransform_->ref();

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setZAxisTransform( nz, tskr );

    return true;
}


bool HorizonDisplay::setChannels2RGBA( visBase::TextureChannel2RGBA* t )
{
    RefMan<visBase::TextureChannel2RGBA> dummy( t );
    if ( sections_.size()!=1 )
	return EMObjectDisplay::setChannels2RGBA( t );

    EMObjectDisplay::setChannels2RGBA( 0 );
    sections_[0]->setChannels2RGBA( t );

    return true;
}


visBase::TextureChannel2RGBA* HorizonDisplay::getChannels2RGBA()
{
    return sections_.size()
	? sections_[0]->getChannels2RGBA()
	: EMObjectDisplay::getChannels2RGBA();
}


const visBase::TextureChannel2RGBA* HorizonDisplay::getChannels2RGBA() const
{ return const_cast<HorizonDisplay*>(this)->getChannels2RGBA(); }


void HorizonDisplay::setSceneEventCatcher(visBase::EventCatcher* ec)
{
    EMObjectDisplay::setSceneEventCatcher( ec );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->setSceneEventCatcher( ec );
	if ( ec && scene_ )
	    sections_[idx]->setRightHandSystem( scene_->isRightHandSystem() );
    }

    for ( int idx=0; idx<intersectiondata_.size(); idx++ )
	intersectiondata_[idx]->setSceneEventCatcher( ec );
}


EM::PosID HorizonDisplay::findClosestNode( const Coord3& pickedpos ) const
{
    const mVisTrans* ztrans = scene_->getTempZStretchTransform();
    Coord3 newpos;
    ztrans->transformBack( pickedpos, newpos );
    if ( transformation_ )
	transformation_->transformBack( newpos );

    const BinID pickedbid = s3dgeom_->transform( newpos.getXY() );
    const EM::PosID pickedsubid = EM::PosID::getFromRowCol( pickedbid );
    TypeSet<EM::PosID> closestnodes;
    for ( int idx=sids_.size()-1; idx>=0; idx-- )
    {
	if ( !emobject_->isDefined( pickedsubid ) )
	    continue;
	closestnodes += pickedsubid;
    }

    if ( closestnodes.isEmpty() )
	return EM::PosID::getInvalid();

    EM::PosID closestnode = closestnodes[0];
    float mindist = mUdf(float);
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emobject_->getPos( closestnodes[idx] );
	Coord3 displaypos;
	mVisTrans::transform( transformation_, coord, displaypos );
	mVisTrans::transform( ztrans, displaypos );

	const float dist = displaypos.distTo<float>( pickedpos );
	if ( !idx || dist<mindist )
	{
	    closestnode = closestnodes[idx];
	    mindist = dist;
	}
    }

    return closestnode;
}


void HorizonDisplay::removeEMStuff()
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	removeChild( sections_[idx]->osgNode() );
	sections_[idx]->unRef();
    }

    sections_.erase();
    sids_.erase();
    intersectiondata_.erase();
    EMObjectDisplay::removeEMStuff();
}


bool HorizonDisplay::setEMObject( const DBKey& newid, TaskRunner* tskr )
{
    return EMObjectDisplay::setEMObject( newid, tskr );
}


StepInterval<int> HorizonDisplay::geometryRowRange() const
{
    mDynamicCastGet(const EM::Horizon3D*, surface, emobject_ );
    if ( !surface ) return parrowrg_;

    return surface->geometry().rowRange();
}


StepInterval<int> HorizonDisplay::geometryColRange() const
{
    mDynamicCastGet(const EM::Horizon3D*,horizon3d,emobject_);
    return horizon3d ? horizon3d->geometry().colRange() : parcolrg_;
}


bool HorizonDisplay::updateFromEM( TaskRunner* tskr )
{
    if ( !EMObjectDisplay::updateFromEM( tskr ) )
	return false;
    updateSingleColor();
    return true;
}


void HorizonDisplay::updateFromMPE()
{
    if ( geometryRowRange().nrSteps()<=1 || geometryColRange().nrSteps()<=1 )
	setResolution( 0, 0 ); //Automatic resolution
    EMObjectDisplay::updateFromMPE();
}


void HorizonDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    const Attrib::SelSpec* selspec = getSelSpec( idx );
	    if ( selspec->id() == Attrib::SelSpec::cNoAttribID() )
	    {
		usestexture_ = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture_ = yn;

    updateSingleColor();

    if ( trigger )
	changedisplay.trigger();
}


bool HorizonDisplay::usesTexture() const
{ return usestexture_; }


bool HorizonDisplay::canShowTexture() const
{
    return validtexture_ && isAnyAttribEnabled() && !displayedOnlyAtSections();
}


bool HorizonDisplay::showsTexture() const
{
    return canShowTexture() && usesTexture();
}


bool HorizonDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


bool HorizonDisplay::canHaveMultipleAttribs() const
{ return true; }


int HorizonDisplay::nrTextures( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() || !sections_.size() )
	return 0;

    return sections_[0]->nrVersions( channel );
}


void HorizonDisplay::selectTexture( int channel, int textureidx )
{
    curtextureidx_ = textureidx;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->selectActiveVersion( channel, textureidx );
}


int HorizonDisplay::selectedTexture( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() || !sections_.size() )
	return 0;

    return sections_[0]->activeVersion( channel );
}


SurveyObject::AttribFormat HorizonDisplay::getAttributeFormat( int ) const
{
    return sections_.size() ? SurveyObject::RandomPos : SurveyObject::None;
}


int HorizonDisplay::nrAttribs() const
{ return as_.size(); }


bool HorizonDisplay::canAddAttrib( int nr ) const
{
    if ( !sections_.size() )
	return false;

    const int maxnr =  sections_[0]->getChannels2RGBA()->maxNrChannels();
    if ( !maxnr ) return true;

    return nrAttribs()+nr<=maxnr;
}


bool HorizonDisplay::canRemoveAttrib() const
{
    if ( !sections_.size() || MPE::engine().getState()==MPE::engine().Started )
	return false;

    return nrAttribs()>=sections_[0]->getChannels2RGBA()->minNrChannels();
}


bool HorizonDisplay::canBeRemoved()const
{
    if ( !sections_.size() || MPE::engine().getState()==MPE::engine().Started )
	return false;

    return true;
}


void HorizonDisplay::setAttribShift( int channel, const TypeSet<float>& shifts )
{
    (*shifts_[channel]) = shifts;
}


bool HorizonDisplay::addAttrib()
{
    as_ += new Attrib::SelSpecList( 1, Attrib::SelSpec() );
    TypeSet<float> shift;
    shift += 0.0;
    curshiftidx_ += 0;
    BufferStringSet* attrnms = new BufferStringSet();
    attrnms->setNullAllowed();
    userrefs_ += attrnms;
    enabled_ += true;
    shifts_ += new TypeSet<float>;
    dispdatapackids_ += new TypeSet<DataPack::ID>;
    coltabmappers_ += new ColTab::Mapper;
    coltabsequences_ += ColTab::SeqMGR().getDefault();

    const int curchannel = coltabmappers_.size()-1;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->addChannel();
	sections_[idx]->setColTabSequence( curchannel,
		*coltabsequences_[curchannel] );

	sections_[idx]->setColTabMapper( curchannel,
		*coltabmappers_[curchannel], SilentTaskRunnerProvider() );
    }

    return true;
}


bool HorizonDisplay::removeAttrib( int channel )
{
    if ( channel<0 || channel>=nrAttribs() )
       return true;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->removeChannel( channel );

    curshiftidx_.removeSingle( channel );
    delete userrefs_.removeSingle( channel );
    enabled_.removeSingle( channel );
    delete shifts_.removeSingle( channel );

    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[channel];
    for ( int idy=dpids.size()-1; idy>=0; idy-- )
	DPM(DataPackMgr::FlatID()).unRef( dpids[idy] );
    delete dispdatapackids_.removeSingle( channel );

    coltabmappers_.removeSingle( channel );
    coltabsequences_.removeSingle( channel );
    delete as_.removeSingle( channel );

    for ( int chan=channel; chan<nrAttribs(); chan++ )
    {
	for ( int idx=0; idx<sections_.size(); idx++ )
	    sections_[idx]->setColTabSequence( chan, *coltabsequences_[chan] );
    }

    return true;
}


bool HorizonDisplay::swapAttribs( int a0, int a1 )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->swapChannels( a0, a1 );

    coltabmappers_.swap( a0, a1 );
    coltabsequences_.swap( a0, a1 );

    as_.swap( a0, a1 );
    enabled_.swap( a0, a1 );
    shifts_.swap( a0, a1 );
    curshiftidx_.swap( a0, a1 );
    userrefs_.swap( a0, a1 );
    dispdatapackids_.swap( a0, a1 );
    return true;
}


void HorizonDisplay::setAttribTransparency( int channel, unsigned char nt )
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setTransparency( channel, nt );
}


unsigned char HorizonDisplay::getAttribTransparency( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return 0;

    return sections_.size() ? sections_[0]->getTransparency(channel) : 0;
}


void HorizonDisplay::enableAttrib( int channelnr, bool yn )
{
    enabled_[channelnr] = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->getChannels2RGBA()->setEnabled( channelnr, yn );

    updateSingleColor();
}


bool HorizonDisplay::isAttribEnabled( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return false;

    return enabled_[channel];
}


void HorizonDisplay::allowShading( bool yn )
{
    allowshading_ = yn;
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
	sections_[idx]->getChannels2RGBA()->allowShading( yn );
}


const Attrib::SelSpec* HorizonDisplay::getSelSpec(
				int channel, int version ) const
{
    return as_.validIdx(channel) && as_[channel]->validIdx(version)
		? &(*as_[channel])[version] : 0;
}


const Attrib::SelSpecList* HorizonDisplay::getSelSpecs( int channel ) const
{
    return as_.validIdx(channel) ? as_[channel] : 0;
}


void HorizonDisplay::setSelSpecs( int channel, const Attrib::SelSpecList& as )
{
    SurveyObject::setSelSpecs( channel, as );

    if ( !as_.validIdx(channel) )
	return;

    *as_[channel] = as;

    BufferStringSet* attrnms = new BufferStringSet();
    for ( int idx=0; idx<as.size(); idx++ )
	attrnms->add( as[idx].userRef() );
    delete userrefs_.replace( channel, attrnms );
}


class ZValSetter : public ParallelTask
{
public:
ZValSetter( BinnedValueSet& bivs, int zcol, ZAxisTransform* zat )
    : ParallelTask()
    , bivs_(bivs), zcol_(zcol), zat_(zat)
{
    hs_.set( bivs_.inlRange(), bivs_.crlRange() );
}


od_int64 nrIterations() const	{ return hs_.totalNr(); }

protected:

bool doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const BinID bid = hs_.atIndex( idx );
	BinnedValueSet::SPos pos = bivs_.findOccurrence( bid );
	if ( !pos.isValid() ) continue;

	float* vals = bivs_.getVals(pos);
	if ( !vals ) continue;

	if ( !zat_ )
	    vals[zcol_] = vals[0];
	else
	{
	    const TrcKey tk( bivs_.geomSystem(), bid );
	    vals[zcol_] = zat_->transformTrc( tk, vals[0] );
	}
    }

    return true;
}

    BinnedValueSet&	bivs_;
    int			zcol_;
    ZAxisTransform*	zat_;
    TrcKeySampling	hs_;

};


void HorizonDisplay::setDepthAsAttrib( int channel )
{
    if ( !as_.validIdx(channel) )
	return;

    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );

    Attrib::SelSpec& as = (*as_[channel])[0];
    const bool attribwasdepth = FixedString(as.userRef())==sKeyZValues();
    as.set( sKeyZValues(), Attrib::SelSpec::cNoAttribID(), false, "" );
    if ( !attribwasdepth )
    {
	ConstRefMan<ColTab::Sequence> seq =
		ColTab::SeqMGR().getAny( OD::defSurfaceDataColSeqName() );
	setColTabSequence( channel, *seq, 0 );
	RefMan<ColTab::Mapper> mapper = new ColTab::Mapper;
	setColTabMapper( channel, *mapper, 0 );
    }

    TypeSet<DataPointSet::DataRow> pts;
    ObjectSet<DataColDef> defs;
    RefMan<DataPointSet> positions = new DataPointSet( pts, defs, false, true );

    const DataColDef siddef( sKeySectionID() );
    if ( positions->dataSet().findColDef(siddef,PosVecDataSet::NameExact)==-1 )
	positions->dataSet().add( new DataColDef(siddef) );

    const DataColDef zvalsdef( sKeyZValues() );
    if ( positions->dataSet().findColDef(zvalsdef,PosVecDataSet::NameExact)
		< 0 )
	positions->dataSet().add( new DataColDef(zvalsdef) );

    getRandomPos( *positions, 0 );

    if ( !positions->size() ) return;

    BinnedValueSet& bivs = positions->bivSet();
    if ( bivs.nrVals()!=3 )
    {
	pErrMsg( "Hmm" );
	return;
    }

    int zcol= positions->dataSet().findColDef( zvalsdef,
						PosVecDataSet::NameExact );
    if ( zcol==-1 )
	zcol = 2;

    ZValSetter setter( bivs, zcol, zaxistransform_ );
    setter.execute();

    setRandomPosData( channel, positions, SilentTaskRunnerProvider() );
}


void HorizonDisplay::getRandomPos( DataPointSet& data, TaskRunner* tskr ) const
{
    //data.bivSet().allowDuplicateBids(false);
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->getDataPositions( data, getTranslation().z_,
					  sids_[idx], tskr );

    data.dataChanged();
}


void HorizonDisplay::getRandomPosCache( int channel, DataPointSet& data ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    data.clearData();
    for ( int idx=0; idx<userrefs_[channel]->size(); idx++ )
	data.dataSet().add( new DataColDef(userrefs_[channel]->get(idx)) );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	const BinnedValueSet* cache = sections_[idx]->getCache( channel );
	if ( cache )
	{
	    data.bivSet().setNrVals( cache->nrVals() );
	    data.bivSet().append( *cache );
	}
    }

    data.dataChanged();
}


void HorizonDisplay::updateSingleColor()
{
    const bool usesinglecol = !showsTexture();
    const Color col = usesinglecol ? nontexturecol_ : Color::White();
    material_->setColor( col );
    if ( intersectionlinematerial_ )
	intersectionlinematerial_->setColor( nontexturecol_ );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->useChannel( !usesinglecol );
	sections_[idx]->setWireframeColor( col );
    }
}


bool HorizonDisplay::usesColor() const
{
    return !showsTexture() || displaysIntersectionLines();
}


void HorizonDisplay::setRandomPosData( int channel, const DataPointSet* data,
				       const TaskRunnerProvider& trprov )
{
    if ( channel<0 || channel>=nrAttribs() || sections_.isEmpty() )
       return;

    if ( !data || !data->size() )
    {
	validtexture_ = false;
	updateSingleColor();
	return;
    }

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setTextureData( channel, data, sids_[idx], trprov );

    coltabmappers_[channel] = &sections_[0]->getColTabMapper( channel );

    // TODO PrIMPL do a better version of this hack and then centralised
    if ( coltabmappers_[channel]->distribution().isEmpty() )
    {
	DPSValueSeries dpsvs( *data, data->nrCols()-1 );
	RangeLimitedDataDistributionExtracter<float> extr( dpsvs, data->size(),
							    trprov );
	const_cast<FloatDistrib&>(
	    coltabmappers_[channel]->distribution()) = *extr.getDistribution();
    }

    validtexture_ = true;
    updateSingleColor();

    BufferStringSet* attrnms = new BufferStringSet();
    for ( int idx=0; idx<data->nrCols(); idx++ )
	attrnms->add( data->colDef(idx).name_ );
    delete userrefs_.replace( channel, attrnms );

    createDisplayDataPacks( channel, data );
}


void HorizonDisplay::createDisplayDataPacks(
			int channel, const DataPointSet* data )
{
    if ( !data || !data->size() || sections_.isEmpty() ) return;

    const StepInterval<int> dispinlrg = sections_[0]->displayedRowRange();
    const StepInterval<int> dispcrlrg = sections_[0]->displayedColRange();
    const BinID step( dispinlrg.step, dispcrlrg.step );
    sections_[0]->setTextureRange( dispinlrg, dispcrlrg );

    const DataColDef sidcoldef( sKeySectionID() );
    const int sidcol =
	data->dataSet().findColDef(sidcoldef,PosVecDataSet::NameExact);
    const int nrfixedcols = data->nrFixedCols();
    const int shift = data->validColID(sidcol) ? nrfixedcols+1 : nrfixedcols;
    const BinnedValueSet* cache =
	sections_.isEmpty() ? 0 : sections_[0]->getCache( channel );
    if ( !cache )
	return;

    const int nrversions = cache->nrVals()-shift;

    TypeSet<DataPack::ID> dpids;
    const char* catnm = "Horizon Data";
    const CubeHorSubSel chss( dispinlrg, dispcrlrg );

    for ( int idx=0; idx<nrversions; idx++ )
    {
	mDeclareAndTryAlloc(BIDValSetArrAdapter*, bvsarr,
			    BIDValSetArrAdapter(*cache,idx+shift,step));
	if ( !bvsarr )
	    continue;

	mDeclareAndTryAlloc(MapDataPack*,mapdp,MapDataPack(catnm,bvsarr));
	if ( !mapdp )
	    continue;

	const Attrib::SelSpec* as = getSelSpec( channel, idx );
	if ( as )
	    mapdp->setName( as->userRef() );

	mapdp->setPositions( chss );
	DPM(DataPackMgr::FlatID()).add( mapdp );
	dpids += mapdp->id();
    }

    setDisplayDataPackIDs( channel, dpids );
}


bool HorizonDisplay::hasStoredAttrib( int channel ) const
{
    const Attrib::SelSpec* selspec = getSelSpec( channel );
    return selspec->id()==Attrib::SelSpec::cOtherAttribID() &&
	   !FixedString(selspec->userRef()).isEmpty();
}


bool HorizonDisplay::hasDepth( int channel ) const
{ return getSelSpec(channel)->id()==Attrib::SelSpec::cNoAttribID(); }


Coord3 HorizonDisplay::getTranslation() const
{
    if ( !translation_ ) return Coord3(0,0,0);

    const Coord3 current = translation_->getTranslation();

    Coord3 origin( 0, 0, 0 );
    Coord3 shift( current );
    shift  *= -1;

    mVisTrans::transformBack( transformation_, origin );
    mVisTrans::transformBack( transformation_, shift );

    const Coord3 translation = origin - shift;

    return translation;
}


void HorizonDisplay::setTranslation( const Coord3& nt )
{
     if ( !nt.isDefined() )
	return;

    Coord3 origin( 0, 0, 0 );
    Coord3 aftershift( nt );
    aftershift.z_ *= -1;

    mVisTrans::transform( transformation_, origin );
    mVisTrans::transform( transformation_, aftershift );

    const Coord3 shift = origin - aftershift;

    translation_->setTranslation( shift );
    translationpos_ = nt;
    setOnlyAtSectionsDisplay( displayonlyatsections_ );		/* retrigger */
}


void HorizonDisplay::setSectionDisplayRestore( bool yn )
{
    oldsectionids_.erase();
    olddisplayedrowranges_.erase();
    olddisplayedcolranges_.erase();
    deepUnRef( oldhortexhandlers_ );

    if ( yn )
    {
	for ( int idx=0; idx<sids_.size(); idx++ )
	{
	    oldsectionids_ += sids_[idx];
	    if ( !sections_.validIdx(idx) ) continue;
	    olddisplayedrowranges_ += sections_[idx]->displayedRowRange();
	    olddisplayedcolranges_ += sections_[idx]->displayedColRange();
	    oldhortexhandlers_ += &sections_[idx]->getTextureHandler();
	    oldhortexhandlers_.last()->ref();
	}
    }
}


void HorizonDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( sections_[idx]->osgNode() );
    sections_.removeSingle( idx )->unRef();
    secnames_.removeSingle( idx );
    sids_.removeSingle( idx );
}


bool HorizonDisplay::addSection( const EM::SectionID& sid, TaskRunner* tskr )
{
    RefMan<visBase::HorizonSection> surf = visBase::HorizonSection::create();
    surf->ref();
    surf->setDisplayTransformation( transformation_ );
    surf->setZAxisTransform( zaxistransform_, tskr );
    if ( scene_ ) surf->setRightHandSystem( scene_->isRightHandSystem() );

    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    mDynamicCastGet( EM::Horizon3D*, horizon, emobject_ );
    surf->setSurface( horizon->geometry().geometryElement(), true, tskr );
    if ( !emobject_->isEmpty() && tskr && !tskr->execResult() )
    {
	surf->unRef();
	return false;
    }

    surf->setResolution( resolution_-1, tskr );
    surf->setMaterial( 0 );

    const int oldsecidx = oldsectionids_.indexOf( sid );
    if ( oldsecidx>=0 )
    {
	if ( surf->displayedRowRange()!=olddisplayedrowranges_[oldsecidx] ||
	     surf->displayedColRange()!=olddisplayedcolranges_[oldsecidx] )
	{
	    surf->setDisplayRange( olddisplayedrowranges_[oldsecidx],
				   olddisplayedcolranges_[oldsecidx] );
	}

	surf->setTextureHandler( *oldhortexhandlers_[oldsecidx] );
    }
    else // initialize texture handler newly created by horizon section
    {
	if ( sections_.isEmpty() && channel2rgba_ )
	{
	    surf->setChannels2RGBA( channel2rgba_ );
	    EMObjectDisplay::setChannels2RGBA( 0 );
	}

	while ( surf->nrChannels()<nrAttribs() )
	    surf->addChannel();

	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    surf->setColTabMapper( idx, *coltabmappers_[idx],
				    SilentTaskRunnerProvider() );
	    surf->setColTabSequence( idx, *coltabsequences_[idx] );
	    surf->getChannels2RGBA()->setEnabled( idx, enabled_[idx] );
	}

	surf->getChannels2RGBA()->allowShading( allowshading_ );
	surf->getChannels()->enableTextureInterpolation( enabletextureinterp_ );
    }

    addChild( surf->osgNode() );

    surf->turnOn( !displayonlyatsections_ );

    sections_ += surf;
    secnames_ += emobject_->name();

    sids_ += sid;
    hasmoved.trigger();

    displaysSurfaceGrid( displaysurfacegrid_ );

    return true;
}


BufferString HorizonDisplay::getSectionName( int secidx ) const
{
    if ( secidx >=secnames_.size() )
	return BufferString();

    return secnames_[secidx];
}


void HorizonDisplay::enableTextureInterpolation( bool yn )
{
    if ( enabletextureinterp_==yn )
	return;

    enabletextureinterp_ = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( !sections_[idx] || !sections_[idx]->getChannels() )
	    continue;

	sections_[idx]->getChannels()->enableTextureInterpolation( yn );

    }
}


void HorizonDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->turnOn(!yn);

    EMObjectDisplay::setOnlyAtSectionsDisplay( yn );

    displayonlyatsections_ = yn;

    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobject_ )
    if ( !hor3d ) return;
    const bool showlock = showlock_ && hor3d->hasLockedNodes();

    if ( lockedpts_ && lockedpts_->size()>0 )
	lockedpts_->turnOn( showlock && !displayonlyatsections_ );

    if ( sectionlockedpts_ )
	sectionlockedpts_->turnOn( showlock && displayonlyatsections_ );
}


visBase::Material* HorizonDisplay::getMaterial()
{
    return material_;
}


void HorizonDisplay::setIntersectLineMaterial( visBase::Material* nm )
{
    unRefAndZeroPtr( intersectionlinematerial_ );

    intersectionlinematerial_ = nm;

    refPtr( intersectionlinematerial_ );

    for ( int idx=0; idx<intersectiondata_.size(); idx++ )
	intersectiondata_[idx]->setMaterial( intersectionlinematerial_ );
}


void HorizonDisplay::emChangeCB( CallBacker* cb )
{
    if ( cb )
    {
	mCBCapsuleUnpack( EM::ObjectCallbackData,cbdata, cb );
	emchangedata_.addCallBackData( new EM::ObjectCallbackData(cbdata) );
    }

    mEnsureExecutedInMainThread( HorizonDisplay::emChangeCB );
    for ( int idx=0; idx<emchangedata_.size(); idx++ )
    {
	const EM::ObjectCallbackData* cbdata =
        emchangedata_.getCallBackData( idx );
	if ( !cbdata )
	    continue;
	EMObjectDisplay::handleEmChange( *cbdata );
	handleEmChange( *cbdata );
    }

    emchangedata_.clearData();
    updateSingleColor();
}


void HorizonDisplay::handleEmChange(const EM::ObjectCallbackData& cbdata)
{
    if ( cbdata.changeType()==EM::Object::cPositionChange() )
    {
	validtexture_ = false;
	const int idx = sids_.indexOf( 0 );
	if ( idx>=0 && idx<sections_.size() )
	    sections_[idx]->inValidateCache(-1);
    }
    else if ( cbdata.changeType()==EM::Object::cPrefColorChange() )
    {
	nontexturecol_ = emobject_->preferredColor();
	setLineStyle( emobject_->preferredLineStyle() );
    }
    else if ( cbdata.changeType()==EM::Object::cSelColorChange() )
    {
	mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ )
	if ( hor3d )
	{
	    if ( selections_ && selections_->getMaterial() )
		selections_->getMaterial()->setColor(
		hor3d->selectionColor() );
	}
    }
    else if ( cbdata.changeType()==EM::Object::cParentColorChange() )
    {
	mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ )
	if ( hor3d )
	{
	    if ( parentline_ && parentline_->getMaterial() )
		parentline_->getMaterial()->setColor( hor3d->getParentColor() );
	}
    }
    else if ( cbdata.changeType()==EM::Object::cSelectionChange() )
    {
	// TODO: Should be made more general, such that it also works for
	// polygon selections
	selectChildren();
    }
    else if ( cbdata.changeType()==EM::Object::cLockChange() )
    {
	// if it is unlocked, we need set all lockedpts to unlock.
	// if it is locked, we do nothing
	mDynamicCastGet( const EM::Horizon3D*, hor3d, emobject_ )
	const bool locked = hor3d ? hor3d->hasLockedNodes() : false;
	if ( locked && !displayonlyatsections_ )
	{
	    updateLockedPointsColor();
	    return;
	}

	if ( !locked )
	{
	    if ( lockedpts_ )
	    {
		lockedpts_->removeAllPoints();
		lockedpts_->removeAllPrimitiveSets();
	    }
	    if ( sectionlockedpts_ )
	    {
		sectionlockedpts_->removeAllPoints();
		sectionlockedpts_->removeAllPrimitiveSets();
	    }
	}
	else
	{
	    calculateLockedPoints(); // re-compute locked points
	    hasmoved.trigger(); // display locked points on section
	    showLocked( showlock_ );
	}
    }
    else if ( cbdata.changeType()==EM::Object::cLockColorChange() )
    {
	updateLockedPointsColor();
    }
}


void HorizonDisplay::updateLockedPointsColor()
{
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobject_ )
    if ( !hor3d ) return;

    if ( lockedpts_ && lockedpts_->getMaterial() )
	lockedpts_->getMaterial()->setColor( hor3d->getLockColor() );

    if ( sectionlockedpts_ && sectionlockedpts_->getMaterial() )
    {
	sectionlockedpts_->getMaterial()->setColor(
	    hor3d->getLockColor() );
    }
}


Color HorizonDisplay::getColor() const
{
    return nontexturecol_;
}


int HorizonDisplay::getChannelIndex( const char* nm ) const
{
    for ( int idx=0; idx<nrAttribs(); idx++ )
	if ( FixedString(getSelSpec(idx)->userRef()) == nm )
	    return idx;

    return -1;
}


void HorizonDisplay::updateAuxData()
{
    const int depthidx = getChannelIndex( sKeyZValues() );
    if ( depthidx != -1 )
	setDepthAsAttrib( depthidx );

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject_)
    if ( !hor3d ) return;

    const BinnedValueSet& auxdata = hor3d->auxdata.getData();
    if ( auxdata.isEmpty() )
	return;

    float auxvals[3];
    auxvals[0] = mUdf(float);
    auxvals[1] = 0;
    for ( int idx=0; idx<auxdata.nrVals(); idx++ )
    {
	const char* auxdatanm = hor3d->auxdata.auxDataName( idx );
	if ( !auxdatanm )
	    continue;
	const int cidx = getChannelIndex( auxdatanm );
	if ( cidx==-1 )
	    continue;

	RefMan<DataPointSet> dps = new DataPointSet( false, true );
	dps->dataSet().add( new DataColDef(sKeySectionID()) );
	dps->dataSet().add( new DataColDef(auxdatanm) );

	BinnedValueSet::SPos pos;
	while ( auxdata.next(pos) )
	{
	    auxvals[2] = auxdata.getVal( pos, idx );
	    dps->bivSet().add( BinID(auxdata.getIdxPair(pos)), auxvals );
	}

	dps->dataChanged();
	setRandomPosData( cidx, dps, SilentTaskRunnerProvider() );
	selectTexture( cidx, 0 );
    }
}


int HorizonDisplay::nrResolutions() const
{
    return sections_.size() ? sections_[0]->nrResolutions()+1 : 1;
}


int HorizonDisplay::getResolution() const
{
    return sections_.size() ? sections_[0]->currentResolution()+1 : 0;
}


uiWord HorizonDisplay::getResolutionName( int idx ) const
{
    if ( idx < 0 || idx > (int)OD::cMinSurfaceResolution() )
	{ pErrMsg("Out of range"); idx = 0; }
    return OD::getSurfaceResolutionDispStr( (OD::SurfaceResolution)idx );
}


bool HorizonDisplay::displaysSurfaceGrid() const
{
    if ( sections_.size() )
       return  sections_[0]->isWireframeDisplayed();
    return false;
}

void HorizonDisplay::setResolution( int res, TaskRunner* tskr )
{
    resolution_ = (char)res;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setResolution( resolution_-1, tskr );
}


void HorizonDisplay::displaysSurfaceGrid( bool yn )
{
    displaysurfacegrid_ = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->enableGeometryTypeDisplay( WireFrame, yn );
    requestSingleRedraw();
}

const ColTab::Sequence& HorizonDisplay::getColTabSequence( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return SurveyObject::getColTabSequence(channel);

    return sections_.size()
	? sections_[0]->getColTabSequence( channel )
	: *coltabsequences_[channel];
}


bool HorizonDisplay::canSetColTabSequence() const
{
    if ( !usesTexture() || !sections_.size() ||
	 !sections_[0]->getChannels2RGBA() )
	return false;

    return sections_[0]->getChannels2RGBA()->canSetSequence();
}


void HorizonDisplay::setColTabSequence( int chan, const ColTab::Sequence& seq,
					TaskRunner* )
{
    if ( chan<0 || chan>=nrAttribs() )
       return;

    coltabsequences_[chan] = ConstRefMan<ColTab::Sequence>( &seq );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setColTabSequence( chan, seq );
}


void HorizonDisplay::setColTabMapper( int channel, const ColTab::Mapper& mpr,
				      TaskRunner* tskr )
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    if ( coltabmappers_.validIdx(channel) )
	coltabmappers_[channel] = &mpr;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setColTabMapper( channel, mpr,
					 ExistingTaskRunnerProvider(tskr) );
}


const ColTab::Mapper& HorizonDisplay::getColTabMapper( int ch ) const
{
    if ( ch<0 || ch>=nrAttribs() )
       return *new ColTab::Mapper;

    return *coltabmappers_[ch];
}


float HorizonDisplay::calcDist( const Coord3& pickpos ) const
{
    if ( !emobject_ ) return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pickpos, xytpos );
    mDynamicCastGet(const EM::Horizon3D*,hor,emobject_)
    if ( hor )
    {
	const BinID bid = s3dgeom_->transform( xytpos.getXY() );
	const EM::PosID bidid = EM::PosID::getFromRowCol( bid );
	TypeSet<Coord3> positions;
	for ( int idx=sids_.size()-1; idx>=0; idx-- )
	{
	    if ( !emobject_->isDefined( bidid ) )
		continue;

	    positions += emobject_->getPos( bidid );
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<positions.size(); idx++ )
	{
	    const float zfactor = scene_
		? scene_->getZScale()
		: s3dgeom_->zScale();
	    const Coord3& pos = positions[idx] + getTranslation()/zfactor;
	    const float dist = (float) fabs(xytpos.z_-pos.z_);
	    if ( dist < mindist ) mindist = dist;
	}

	return mindist;
    }

    return mUdf(float);
}


float HorizonDisplay::maxDist() const
{
    return s3dgeom_->zRange().step;
}


visBase::HorizonSection* HorizonDisplay::getHorizonSection(
					const EM::SectionID& sid )
{
    for ( int idx=0; idx<sids_.size(); idx++ )
    {
	if ( sids_[idx]==sid )
	    return sections_[idx];
    }

    return nullptr;
}


const visBase::HorizonSection* HorizonDisplay::getHorizonSection(
					const EM::SectionID& sid ) const
{
    return const_cast<HorizonDisplay*>(this)->getHorizonSection( sid );
}


EM::SectionID HorizonDisplay::getSectionID( int visid ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( sections_[idx]->id()==visid )
		return sids_[idx];
    }

    return -1;
}


void HorizonDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos, BufferString& val,
				       BufferString& info ) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, pos, val, info );
    if ( !emobject_ || !showsTexture() ) return;

    const EM::SectionID sid =
	EMObjectDisplay::getSectionID(&eventinfo.pickedobjids);

    const int sectionidx = sids_.indexOf( sid );
    if ( sectionidx<0 || sectionidx>=sections_.size() )
	return;

    const BinID bid( s3dgeom_->transform(pos.getXY()) );

    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	const Attrib::SelSpec* selspec = getSelSpec( idx );
	if ( selspec->id().isUnselInvalid() )
	    return;

	if ( !sections_[sectionidx]->getChannels2RGBA()->isEnabled(idx) ||
	      sections_[sectionidx]->getTransparency(idx)==255 )
	    continue;

	const BinnedValueSet* bidvalset = sections_[sectionidx]->getCache(idx);
	if ( !bidvalset || bidvalset->nrVals()<2 ) continue;

	const BinnedValueSet::SPos setpos = bidvalset->find( bid );
	if ( !setpos.isValid() )
	    continue;

	const float* vals = bidvalset->getVals( setpos );
	int curtexture = selectedTexture(idx);
	if ( curtexture>bidvalset->nrVals()-2 ) curtexture = 0;

	const float fval = vals[curtexture+2];
	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !sections_[sectionidx]->getCache(idy) ||
		 !isAttribEnabled(idy) ||
		 sections_[sectionidx]->getTransparency(idy)==255 )
		continue;

		islowest = false;
		break;
	}

	if ( !islowest )
	{
	    const ColTab::Sequence& coltabseq = getColTabSequence( idx );
	    const Color col = coltabseq.color(fval);
	    if ( col!=coltabseq.undefColor() && col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	{
	    val = fval;
	    float attribshift = 0;
	    const TypeSet<float>& attribshifts = *shifts_[idx];
	    const int version = selectedTexture( idx );
	    if ( attribshifts.validIdx(version) )
	    {
		attribshift = attribshifts[version]
			    * SI().zDomain().userFactor();
	    }

	    const float zshift =
	      (float)(getTranslation().z_ * SI().zDomain().userFactor());

	    const bool hasshift = !mIsZero(attribshift,0.1) ||
				  !mIsZero(zshift,0.1);
	    const int nrattribs = nrAttribs();
	    if ( nrattribs>1 || hasshift )
	    {
		BufferString channelstr = "(";
		bool first = true;
		if ( nrattribs > 1 )
		{
		    channelstr += selspec->userRef();
		    first = false;
		}

		if ( hasshift )
		{
		    if ( !first )
			channelstr += ", ";
		    channelstr += "shift=";
		    channelstr += attribshift;
		}

		channelstr += ")";
		val.replaceAt( cValNameOffset(), channelstr );
	    }
	}

	return;
    }
}


void HorizonDisplay::traverseLine( const TrcKeyPath& path,
				   const TypeSet<Coord>& crds,
				   const Interval<float>& zrg,
				   HorizonDisplay::IntersectionData& res ) const
{
    HorizonPathIntersector hpi( *this, path, crds, zrg, res );
    hpi.execute();
}


#define mAddLinePrimitiveSet \
{\
    if ( idxps.size()>=2 )\
    {\
       Geometry::IndexedPrimitiveSet* primitiveset =  \
               Geometry::IndexedPrimitiveSet::create( false );\
       primitiveset->ref();\
       primitiveset->append( idxps.arr(), idxps.size() );\
       line->addPrimitiveSet( primitiveset ); \
       primitiveset->unRef();\
    }\
}


void HorizonDisplay::drawHorizonOnZSlice( const TrcKeyZSampling& tkzs,
			HorizonDisplay::IntersectionData& res ) const
{
    mDynamicCastGet(const EM::Horizon3D*,horizon,emobject_);
    if ( !horizon ) return;

    const float zshift = (float) getTranslation().z_;

    const Geometry::BinIDSurface* geom =
	horizon->geometry().geometryElement();

    const Array2D<float>* field = geom->getArray();
    if ( !field ) return;

    ConstPtrMan<Array2D<float> > myfield;
    if ( zaxistransform_ )
    {
	myfield = field = horizon->createArray2D( zaxistransform_ );
    }

    IsoContourTracer ictracer( *field );
    ictracer.setSampling( geom->rowRange(), geom->colRange() );
    ictracer.selectRectROI( tkzs.hsamp_.inlRange(), tkzs.hsamp_.crlRange() );
    ManagedObjectSet<ODPolygon<float> > isocontours;
    ictracer.getContours( isocontours, tkzs.zsamp_.start-zshift, false );

    TypeSet<int> idxps;
    for ( int cidx=0; cidx<isocontours.size(); cidx++ )
    {
	const ODPolygon<float>& ic = *isocontours[cidx];
	TypeSet<Coord3> curline;

	for ( int vidx=0; vidx<ic.size(); vidx++ )
	{
	    const Geom::Point2D<float> vertex = ic.getVertex( vidx );
	    Coord vrtxcoord( vertex.x_, vertex.y_ );
	    vrtxcoord = s3dgeom_->binID2Coord().transform( vrtxcoord );
	    const Coord3 pos( vrtxcoord, tkzs.zsamp_.start-zshift );
	    curline += pos;
	}

	if ( ic.isClosed() && curline.size() )
	    curline += curline[0];

	res.addLine( curline );
    }
}


HorizonDisplay::IntersectionData*
HorizonDisplay::getOrCreateIntersectionData(
		ObjectSet<IntersectionData>& pool )
{
    IntersectionData* data = 0;
    if ( pool.size() )
    {
	data = pool.pop();
	data->clear();
    }
    else
    {
	data = new IntersectionData( *lineStyle() );
	data->setDisplayTransformation(transformation_);
	if ( intersectionlinematerial_ )
	    data->setMaterial( intersectionlinematerial_ );
	addChild( data->line_->osgNode() );
	addChild( data->markerset_->osgNode() );
    }

    return data;
}


#define mHandleIndex(obj)\
{ if ( obj && obj->id() == objid ) { objidx = idx; return true;}  }

bool HorizonDisplay::isValidIntersectionObject(
    const ObjectSet<const SurveyObject>&objs, int& objidx, int objid ) const
{
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	mHandleIndex( plane )

	mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	mHandleIndex( mped )

	mDynamicCastGet( const RandomTrackDisplay*, rtdisplay, objs[idx] );
	mHandleIndex( rtdisplay )

	mDynamicCastGet( const Seis2DDisplay*, seis2ddisplay, objs[idx] );
	mHandleIndex( seis2ddisplay )
    }

    return false;
}


void HorizonDisplay::updateIntersectionLines(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    mDynamicCastGet(const EM::Horizon3D*,horizon,emobject_);
    if ( !horizon ) return;

    int objidx = -1;
    const bool doall = whichobj==-1 || whichobj==id();
    if ( !doall && !isValidIntersectionObject(objs,objidx,whichobj) )
	return;

    ManagedObjectSet<IntersectionData> lines;
    if ( doall )
    {
	while ( intersectiondata_.size() )
	    lines += intersectiondata_.removeAndTake( 0, false );
    }
    else
    {
	for ( int idx=0; idx<intersectiondata_.size(); idx++ )
	{
	    if ( intersectiondata_[idx]->objid_ == whichobj )
	    {
		removeChild( intersectiondata_[idx]->line_->osgNode() );
		removeChild( intersectiondata_[idx]->markerset_->osgNode() );
		intersectiondata_.removeSingle(idx);
		break;
	    }
	}
    }

    if ( isOn() && (displayonlyatsections_ || displayintersectionlines_) )
    {
	const int size = doall ? objs.size() : 1;
	for ( int idx=0; idx<size; idx++ )
	{
	    objidx = doall ? idx : objidx;
	    mDynamicCastGet(const VisualObject*, vo, objs[objidx] );
	    if ( !vo )
		continue;

            const TrcKeyZSampling trzs = objs[objidx]->getTrcKeyZSampling(-1);
            TrcKeyPath trckeypath;
            TypeSet<Coord> trccoords;
            objs[objidx]->getTraceKeyPath( trckeypath, &trccoords );

            if ( trckeypath.isEmpty() && trzs.isEmpty() )
                return;

	    IntersectionData* data = 0;

	    if ( trckeypath.size() )
	    {
		const Interval<float> zrg =
		    objs[objidx]->getDataTraceRange();
		data = getOrCreateIntersectionData( lines );
		data->objid_ = vo->id();
		traverseLine( trckeypath, trccoords, zrg, *data );
		continue;
	    }
	    else
	    {
		if ( mIsZero(trzs.zsamp_.width(),1e-5) )
		{
		    data = getOrCreateIntersectionData( lines );
		    data->objid_ = vo->id();
		    drawHorizonOnZSlice( trzs, *data );
		}
	    }

	    intersectiondata_ += data;
	}

    }

    //These lines were not used, hence remove from scene.
    for ( int idx=0; idx<lines.size(); idx++ )
    {
	removeChild( lines[idx]->line_->osgNode() );
	removeChild( lines[idx]->markerset_->osgNode() );
    }
}


void HorizonDisplay::setLineStyle( const OD::LineStyle& lst )
{
    if ( lst==*lineStyle() )
	return;

    EMObjectDisplay::setLineStyle( lst );

    for ( int idx=0; idx<intersectiondata_.size(); idx++ )
    {
	RefMan<visBase::VertexShape> old =
	    intersectiondata_[idx]->setLineStyle( lst );
	if ( old )
	{
	    removeChild( old->osgNode() );
	    addChild( intersectiondata_[idx]->line_->osgNode());
	}
    }

    const float pixeldensity = intersectiondata_.isEmpty()
			? getDefaultPixelDensity()
			: intersectiondata_[0]->line_->getPixelDensity();
    const float factor = pixeldensity / getDefaultPixelDensity();
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setLineWidth( mNINT32(lst.width_*factor) );
}


void HorizonDisplay::updateSectionSeeds(
	    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    if ( !isOn() )
	return;

    bool refresh = movedobj==-1 || movedobj==id();
    TypeSet<int> verticalsections;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const RandomTrackDisplay*,randomtrack,objs[idx]);
	if ( randomtrack )
	{
	    verticalsections += idx;
	    if ( movedobj==randomtrack->id() )
		refresh = true;
	}

	mDynamicCastGet(const PlaneDataDisplay*,plane,objs[idx]);
	if ( plane && plane->getOrientation()!=OD::ZSlice )
	{
	    verticalsections += idx;
	    if ( movedobj==plane->id() )
		refresh = true;
	}

	mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	if ( mped && mped->isDraggerShown() )
	{
	    TrcKeyZSampling tkzs;
	    if ( mped->getPlanePosition(tkzs) && tkzs.nrZ()!=1 )
	    {
		verticalsections += idx;
		if ( movedobj==mped->id() )
		    refresh = true;
	    }
	}
    }

    if ( !refresh ) return;

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = posattribmarkers_[idx];
	for ( int idy=0; idy<markerset->getCoordinates()->size(); idy++ )
	{
	    markerset->turnMarkerOn( idy,!displayonlyatsections_ );
	    const visBase::Coordinates* markercoords =
					markerset->getCoordinates();
	    if ( markercoords->size() )
	    {
		Coord3 markerpos = markercoords->getPos( idy );
		if ( !markerpos.isDefined() )
		{
		    markerset->turnMarkerOn( idy, false );
		    continue;
		}

		if ( transformation_ )
		     mVisTrans::transform( transformation_,  markerpos );

		if ( zaxistransform_ )
		{
		    markerset->turnMarkerOn( idy,false );
		    continue;
		}

		for ( int idz=0; idz<verticalsections.size(); idz++ )
		{
		    const float dist =
			objs[verticalsections[idz]]->calcDist( markerpos );

		    if ( !mIsUdf(dist) &&
			 dist<objs[verticalsections[idz]]->maxDist() )
		    {
			markerset->turnMarkerOn( idy,true );
			break;
		    }
		}
	    }
	}
    }

    // handle locked points on section
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobject_ )
    if ( !lockedpts_ || lockedpts_->size()<=0 || !displayonlyatsections_ )
	return;

    if ( !sectionlockedpts_ )
    {
        sectionlockedpts_ = visBase::PointSet::create();
	sectionlockedpts_->ref();
	addChild( sectionlockedpts_->osgNode() );
	sectionlockedpts_->setDisplayTransformation( transformation_ );
    }
    else
    {
	sectionlockedpts_->removeAllPoints();
	sectionlockedpts_->removeAllPrimitiveSets();
	sectionlockedpts_->getMaterial()->clear();
    }

    TrcKeyPath alttrckeys;
    for ( int idx=0; idx<verticalsections.size(); idx++ )
    {
	TrcKeyPath trckeypath;
	objs[verticalsections[idx]]->getTraceKeyPath( trckeypath );
	alttrckeys.append( trckeypath );
    }

    TypeSet<int> pidxs;
    LockedPointsPathFinder lockedpointspathfinder( *hor3d, alttrckeys,
						   *sectionlockedpts_, pidxs );
    lockedpointspathfinder.execute();
    if ( pidxs.isEmpty() )
	return;

    Geometry::PrimitiveSet* pointsetps =
	Geometry::IndexedPrimitiveSet::create(true);
    pointsetps->setPrimitiveType( Geometry::PrimitiveSet::Points );
    pointsetps->append( pidxs.arr(), pidxs.size() );
    sectionlockedpts_->addPrimitiveSet( pointsetps );

    if ( hor3d )
	sectionlockedpts_->getMaterial()->setColor( hor3d->getLockColor() );

    lockedpts_->turnOn( false );

    sectionlockedpts_->turnOn( showlock_ && hor3d->hasLockedNodes() &&
	displayonlyatsections_ );
}


void HorizonDisplay::selectParent( const TrcKey& tk )
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobject_)
    if ( !hor3d ) return;

    if ( !parentline_ )
    {
	parentline_ = visBase::PolyLine3D::create();
	parentline_->setMaterial( new visBase::Material() );
	parentline_->getMaterial()->setColor( hor3d->getParentColor() );
	addChild( parentline_->osgNode() );
	parentline_->setDisplayTransformation( transformation_ );
    }
    else
    {
	parentline_->getCoordinates()->setEmpty();
	parentline_->removeAllPrimitiveSets();
    }

    TypeSet<int> idxps;
    int cii = 0;
    TypeSet<TrcKey> parents;
    hor3d->getParents( tk, parents );
    for ( int idx=0; idx<parents.size(); idx++ )
    {
	const TrcKey& curnode = parents[idx];
	const Coord3 crd( s3dgeom_->transform(curnode.binID()),
			    hor3d->getZ(curnode) );
	parentline_->getCoordinates()->addPos( crd );
	idxps.add( cii++ );
    }

    visBase::VertexShape* line = parentline_;
    mAddLinePrimitiveSet;
    showParentLine( true );
}


void HorizonDisplay::initSelectionDisplay( bool erase )
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobject_)
    if ( !selections_ )
    {
	selections_ = visBase::PointSet::create();
	selections_->ref();
	if ( hor3d && selections_->getMaterial() )
	    selections_->getMaterial()->setColor( hor3d->selectionColor() );
	addChild( selections_->osgNode() );
	selections_->setDisplayTransformation( transformation_ );
    }
    else if ( erase )
    {
	selections_->clear();
	selectionids_.setEmpty();
    }
}


void HorizonDisplay::selectChildren()
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobject_)
    Array2D<char>* children = hor3d ? hor3d->getChildren() : 0;
    if ( !children ) return;

    initSelectionDisplay( true );

    const TrcKeySampling tks = hor3d->getTrackingSampling();
    TypeSet<int> pidxs;
    for ( od_int64 gidx=0; gidx<children->totalSize(); gidx++ )
    {
	if ( children->getData()[gidx] == '0' )
	    continue;

	const TrcKey tk = TrcKey( tks.atIndex(gidx) );
	const Coord3 pos = hor3d->getPos( EM::PosID::getFromRowCol(tk.binID()));
	const int pidx = selections_->addPoint( pos );
	selections_->getMaterial()->setColor( hor3d->selectionColor(), pidx);
	pidxs += pidx;
    }

    Geometry::PrimitiveSet* pointsetps =
		Geometry::IndexedPrimitiveSet::create( true );
    pointsetps->setPrimitiveType( Geometry::PrimitiveSet::Points );
    pointsetps->append( pidxs.arr(), pidxs.size() );
    selections_->addPrimitiveSet( pointsetps );
    selections_->turnOn( true );
}


void HorizonDisplay::showParentLine( bool yn )
{
    if ( parentline_ ) parentline_->turnOn( yn );
}


void HorizonDisplay::showSelections( bool yn )
{
    if ( selections_ ) selections_->turnOn( yn );
}


void HorizonDisplay::showLocked( bool yn )
{
    showlock_ = yn;

    if ( showlock_ )
	calculateLockedPoints();

    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobject_ )
    if ( !hor3d ) return;
    const bool showlock = showlock_ && hor3d->hasLockedNodes();

    if ( lockedpts_ )
	lockedpts_->turnOn( showlock && !displayonlyatsections_ );

    if ( sectionlockedpts_ )
	sectionlockedpts_->turnOn( showlock && displayonlyatsections_ );

    updateLockedPointsColor();
}


void HorizonDisplay::calculateLockedPoints()
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobject_)
    const Array2D<char>* lockednodes = hor3d ? hor3d->getLockedNodes() : 0;
    if ( !lockednodes ) return;

    if ( !lockedpts_ )
    {
	lockedpts_ = visBase::PointSet::create();
	lockedpts_->ref();
	addChild( lockedpts_->osgNode() );
	lockedpts_->setDisplayTransformation( transformation_ );
    }
    else
    {
	lockedpts_->removeAllPoints();
	lockedpts_->removeAllPrimitiveSets();
	lockedpts_->getMaterial()->clear();
	if ( lockedpts_->getMaterial() )
	    lockedpts_->getMaterial()->setColor( hor3d->getLockColor() );
    }

    const TrcKeySampling tks = hor3d->getTrackingSampling();
    TypeSet<int> pidxs;
    for ( od_int64 gidx=0; gidx<lockednodes->totalSize(); gidx++ )
    {
	if ( lockednodes->getData()[gidx] == '0' )
	    continue;

	const TrcKey tk = TrcKey( tks.atIndex(gidx) );
	const Coord3 pos = hor3d->getPos( EM::PosID::getFromRowCol(tk.binID()));
	const int pidx = lockedpts_->addPoint( pos );
	pidxs += pidx;
    }

    if ( pidxs.size()==0 )
	return;

    Geometry::PrimitiveSet* pointsetps =
	Geometry::IndexedPrimitiveSet::create( true );
    pointsetps->setPrimitiveType( Geometry::PrimitiveSet::Points );
    pointsetps->append( pidxs.arr(), pidxs.size() );
    lockedpts_->addPrimitiveSet( pointsetps );

}


bool HorizonDisplay::lockedShown() const
{
    const bool lockedshow = lockedpts_ ?
	lockedpts_->size()>0 && lockedpts_->isOn() : false;
    const bool sectionlockedshow =
	sectionlockedpts_ ?
	sectionlockedpts_->size()>0 && sectionlockedpts_->isOn() : false;
    return lockedshow || sectionlockedshow;
}


void HorizonDisplay::updateSelections()
{
    const Selector<Coord3>* sel = scene_->getSelector();
    if ( !sel )
	return;


    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobject_)
    if ( !hor3d || !sel ) return;

    EMObjectDisplay::updateSelections();

    initSelectionDisplay( !ctrldown_ );

    ObjectSet<const Selector<Coord3> > selectors;
    selectors += sel;
    EM::ObjectPosSelector posselector( *hor3d, selectors );
    posselector.execute();

    TypeSet<int> pidxs;
    const TypeSet<EM::PosID>& selids = posselector.getSelected();
    if ( selids.size()<=0 )
	return;

    for ( int idx=0; idx<selids.size(); idx++ )
    {
	const Coord3 pos = hor3d->getPos( selids[idx] );
	if ( pos.isUdf() ) continue;

	const int pidx = selections_->addPoint( pos );
	selectionids_ += selids[idx];
	pidxs += pidx;
    }

    if ( pidxs.size()<=0 )
	return;

    Geometry::PrimitiveSet* pointsetps =
		Geometry::IndexedPrimitiveSet::create( true );
    pointsetps->setPrimitiveType( Geometry::PrimitiveSet::Points );
    pointsetps->append( pidxs.arr(), pidxs.size() );
    selections_->addPrimitiveSet( pointsetps );
    selections_->getMaterial()->setColor( hor3d->selectionColor() );
    selections_->turnOn( true );
}


void HorizonDisplay::clearSelections()
{
    EMObjectDisplay::clearSelections();
    if ( selections_ )
    {
	selections_->clear();
    }
}


void HorizonDisplay::doOtherObjectsMoved(
	const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    otherObjectsMoved( objs, whichobj );
}


void HorizonDisplay::otherObjectsMoved(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    Threads::Locker locker( updatelock_ );
    updateIntersectionLines( objs, whichobj );
    updateSectionSeeds( objs, whichobj );
}


void HorizonDisplay::fillPar( IOPar& par ) const
{
    EMObjectDisplay::fillPar( par );

    if ( emobject_ && !emobject_->isFullyLoaded() )
    {
	par.set( sKeyRowRange(), geometryRowRange() );
	par.set( sKeyColRange(), geometryColRange() );
    }

    par.setYN( sKeyTexture(), usesTexture() );
    par.set( sKeyShift(), getTranslation().z_ );
    par.set( sKeyResolution(), getResolution() );
    par.setYN( sKeySurfaceGrid(), displaysSurfaceGrid() );

    for ( int channel=nrAttribs()-1; channel>=0; channel-- )
    {
	IOPar channelpar;
	(*as_[channel])[0].fillPar( channelpar );

	getColTabMapper(channel).setup().fillPar( channelpar );
	getColTabSequence(channel).fillPar( channelpar );

	channelpar.setYN( sKeyIsOn(), isAttribEnabled(channel) );

	BufferString key = sKeyAttribs();
	key += channel;
	par.mergeComp( channelpar, key );
    }

    par.set( sKeyNrAttribs(), nrAttribs() );
}


bool HorizonDisplay::usePar( const IOPar& par )
{
    if ( !EMObjectDisplay::usePar( par ) )
	 return false;

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    if ( !par.get(sKeyEarthModelID(),parmid_) )
	return false;

    par.get( sKeyRowRange(), parrowrg_ );
    par.get( sKeyColRange(), parcolrg_ );

    if ( !par.getYN(sKeyTexture(),usestexture_) )
	usestexture_ = true;

    int resolution = 0;
    par.get( sKeyResolution(), resolution );
    setResolution( resolution, 0 );

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift(), shift.z_ );
    setTranslation( shift );

    bool displaysurfacegrid =  false;
    par.getYN( sKeySurfaceGrid(), displaysurfacegrid );
    displaysSurfaceGrid( displaysurfacegrid );

    int intersectlinematid;
    if ( par.get(sKeyIntersectLineMaterialID(),intersectlinematid) )
    {
	if ( intersectlinematid==-1 )
	    setIntersectLineMaterial( 0 );
	else
	{
	    DataObject* mat = visBase::DM().getObject( intersectlinematid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(visBase::Material) ) return -1;

	    setIntersectLineMaterial( (visBase::Material*)mat );
	}
    }

    return true;
}


void HorizonDisplay::setDisplayDataPackIDs( int attrib,
				const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *dispdatapackids_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).unRef( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).ref( dpids[idx] );
}


DataPack::ID HorizonDisplay::getDataPackID( int channel ) const
{
    return getDisplayedDataPackID( channel );
}


DataPack::ID HorizonDisplay::getDisplayedDataPackID( int channel ) const
{
    if ( sections_.isEmpty() || !dispdatapackids_.validIdx(channel) )
	return DataPack::cNoID();

    const TypeSet<DataPack::ID>& dpids = *dispdatapackids_[channel];
    const int curversion = sections_[0]->activeVersion( channel );
    return dpids.validIdx(curversion) ? dpids[curversion] : DataPack::cNoID();
}


const visBase::HorizonSection* HorizonDisplay::getSection( int horsecid ) const
{
    return sections_.validIdx( horsecid ) ? sections_[horsecid] : 0;
}


HorizonDisplay* HorizonDisplay::getHorizonDisplay( const DBKey& mid )
{
    TypeSet<int> ids;
    visBase::DM().getIDs( typeid(visSurvey::HorizonDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( HorizonDisplay*, hordisp, dataobj );
	if ( hordisp && mid==hordisp->getDBKey() )
	    return hordisp;
    }
    return 0;
}


void HorizonDisplay::setPixelDensity( float dpi )
{
    EMObjectDisplay::setPixelDensity( dpi );

    for ( int idx=0; idx<intersectiondata_.size(); idx++ )
	intersectiondata_[idx]->setPixelDensity( dpi );
}


void HorizonDisplay::showPosAttrib( int attr, bool yn )
{
    EMObjectDisplay::showPosAttrib( attr, yn );
    setOnlyAtSectionsDisplay( displayonlyatsections_ );	/* retrigger */
}

//==============================================================================

HorizonDisplay::IntersectionData::IntersectionData( const OD::LineStyle& lst )
    : line_( lst.type_==OD::LineStyle::Solid
	? (visBase::VertexShape*) visBase::PolyLine3D::create()
	: (visBase::VertexShape*) visBase::PolyLine::create() )
    , markerset_( visBase::MarkerSet::create() )
    , voiid_(-2)
    , zaxistransform_(0)
    , objid_(0)
{
    line_->ref();
    markerset_->ref();

    line_->setPrimitiveType( Geometry::PrimitiveSet::LineStrips );

    setLineStyle( lst );
}


HorizonDisplay::IntersectionData::~IntersectionData()
{
    if ( zaxistransform_ )
    {
        zaxistransform_->removeVolumeOfInterest( voiid_ );
        unRefAndZeroPtr( zaxistransform_ );
    }

    unRefAndZeroPtr( line_ );
    unRefAndZeroPtr( markerset_ );
}


void HorizonDisplay::IntersectionData::addLine( const TypeSet<Coord3>& crds )
{
    if ( !crds.size() )
        return;

    if ( crds.size()==1 )
    {
        markerset_->addPos( crds[0] );
        return;
    }

    const int start = line_->getCoordinates()->size();
    const int stop = start + crds.size()-1;

    line_->getCoordinates()->setPositions( crds.arr(), crds.size(), start );

    Geometry::RangePrimitiveSet* rps = Geometry::RangePrimitiveSet::create();
    rps->setRange( Interval<int>( start, stop ) );
    line_->addPrimitiveSet( rps );
}


void HorizonDisplay::IntersectionData::clear()
{
    line_->removeAllPrimitiveSets();
    line_->getCoordinates()->setEmpty();
    markerset_->clearMarkers();
}


void HorizonDisplay::IntersectionData::setDisplayTransformation(
						const mVisTrans* tran)
{
    line_->setDisplayTransformation( tran );
    markerset_->setDisplayTransformation( tran );
}


void HorizonDisplay::IntersectionData::updateDataTransform(
        const TrcKeyZSampling& sampling, ZAxisTransform* trans )
{
    if ( zaxistransform_ && zaxistransform_!=trans )
    {
        zaxistransform_->removeVolumeOfInterest( voiid_ );
        unRefAndZeroPtr( zaxistransform_ );
        voiid_ = -2;
    }
    zaxistransform_ = trans;
    if ( zaxistransform_ )
    {
	if ( voiid_==-2 )
	    voiid_ = zaxistransform_->addVolumeOfInterest(sampling,true);
	else
	    zaxistransform_->setVolumeOfInterest( voiid_,sampling,true);

	if ( voiid_>=0 )
	    zaxistransform_->loadDataIfMissing( voiid_,
						SilentTaskRunnerProvider() );
    }
}


void HorizonDisplay::IntersectionData::setPixelDensity( float dpi )
{
    if ( line_ ) line_->setPixelDensity( dpi );
    if ( markerset_ ) markerset_->setPixelDensity( dpi );
}


void HorizonDisplay::IntersectionData::setMaterial( visBase::Material* mat )
{
    if ( line_ ) line_->setMaterial( mat );
    if ( markerset_ ) markerset_->setMaterial( mat );
}


void HorizonDisplay::IntersectionData::setSceneEventCatcher(
						visBase::EventCatcher* ev)
{
    if ( line_ ) line_->setSceneEventCatcher( ev );
    if ( markerset_ ) markerset_->setSceneEventCatcher( ev );
}


RefMan<visBase::VertexShape>
HorizonDisplay::IntersectionData::setLineStyle( const OD::LineStyle& lst )
{
    RefMan<visBase::VertexShape> oldline = 0;
    if ( line_ )
    {
	mDynamicCastGet( visBase::PolyLine3D*,ln3d,line_);

	const bool removelines =
	    (lst.type_==OD::LineStyle::Solid) != ((bool) ln3d );

	if ( removelines )
	{
	    visBase::VertexShape* newline = lst.type_==OD::LineStyle::Solid
		? (visBase::VertexShape*) visBase::PolyLine3D::create()
		: (visBase::VertexShape*) visBase::PolyLine::create();
	    newline->ref();
	    newline->setRightHandSystem( line_->isRightHandSystem() );
	    newline->setDisplayTransformation(
					line_->getDisplayTransformation());
	    newline->getCoordinates()->copyFrom( *line_->getCoordinates() );
	    if ( line_->getMaterial() )
		newline->setMaterial( line_->getMaterial() );

	    oldline = line_;
	    unRefAndZeroPtr( line_ );
	    line_ = newline;
	    line_->ref();
	}

	if ( lst.type_==OD::LineStyle::Solid )
	{
	    ((visBase::PolyLine3D* ) line_ )->setLineStyle( lst );
	}
    }

    markerset_->setScreenSize(((float) lst.width_)/2.0f);

    return oldline;
}


} // namespace visSurvey
