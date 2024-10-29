/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visseis2ddisplay.h"

#include "visevent.h"
#include "vismaterial.h"
#include "vistexturechannels.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "color.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "seisdatapackzaxistransformer.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"
#include "uistrings.h"

//For parsing old pars
#include "attribsel.h"


namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyTextureID()	{ return "Texture ID"; }
const char* Seis2DDisplay::sKeyShowPanel()	{ return "Show panel"; }
const char* Seis2DDisplay::sKeyShowPolyLine()	{ return "Show polyline"; }

Seis2DDisplay::Seis2DDisplay()
    : geometry_(*new PosInfo::Line2DData)
    , maxtrcnrrg_(0,mUdf(int),1)
    , pixeldensity_( getDefaultPixelDensity() )
    , geomchanged_(this)
    , geomidchanged_(this)
{
    ref();
    datapacks_.setNullAllowed();
    transformedpacks_.setNullAllowed();

    geometry_.setZRange( ZSampling(mUdf(float),mUdf(float),1.f) );

    RefMan<visBase::Material> newmat = visBase::Material::create();
    polyline_ = visBase::PolyLine::create();
    polyline_->setMaterial( newmat.ptr() );
    addChild( polyline_->osgNode() );

    RefMan<visBase::DrawStyle> newstyle = visBase::DrawStyle::create();
    polylineds_ = polyline_->addNodeState( newstyle.ptr() );
    polylineds_->setLineStyle(
		    OD::LineStyle(OD::LineStyle::Solid,3,OD::Color::White()) );
    setColor( OD::Color::White() );

    panelstrip_ = visBase::TexturePanelStrip::create();
    addChild( panelstrip_->osgNode() );
    panelstrip_->setTextureChannels( channels_.ptr() );
    panelstrip_->swapTextureAxes();
    panelstrip_->smoothNormals();

    linename_ = visBase::Text2::create();
    addChild( linename_->osgNode() );
    linename_->addText();

    getMaterial()->setColor( OD::Color::White() );
    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.2 );

    setColor( OD::Color(0,150,75) );

    init();
    showPanel( false );
    unRefNoDelete();
}


Seis2DDisplay::~Seis2DDisplay()
{
    removeChild( linename_->osgNode() );
    linename_ = nullptr;
    delete &geometry_;

    transformation_ = nullptr;
    polyline_->removeNodeState( polylineds_.ptr() );
    polylineds_ = nullptr;
    polyline_ = nullptr;
    panelstrip_ = nullptr;
    setZAxisTransform( nullptr, nullptr );
}


void Seis2DDisplay::setColor( OD::Color nc )
{
    polyline_->getMaterial()->setColor( nc );
}


OD::Color Seis2DDisplay::getColor() const
{
    return polyline_->getMaterial()->getColor();
}


const OD::LineStyle* Seis2DDisplay::lineStyle() const
{ return &polylineds_->lineStyle(); }


void Seis2DDisplay::setLineStyle( const OD::LineStyle& ls )
{
    polylineds_->setLineStyle( ls );
    requestSingleRedraw();
}


void Seis2DDisplay::showPanel( bool yn )
{
    panelstrip_->turnOn( yn );
    geomchanged_.trigger();
}


void Seis2DDisplay::showPolyLine( bool yn )
{ polyline_->turnOn( yn ); }

bool Seis2DDisplay::isPanelShown() const
{ return panelstrip_->isOn(); }

bool Seis2DDisplay::isPolyLineShown() const
{ return polyline_->isOn(); }


void Seis2DDisplay::enableAttrib( int attrib, bool yn )
{
    MultiTextureSurveyObject::enableAttrib( attrib, yn );
    geomchanged_.trigger();
}


void Seis2DDisplay::setGeomID( const Pos::GeomID& geomid )
{
    geomid_ = geomid;
    BufferString lnm = Survey::GM().getName( geomid_ );
    if ( lnm.isEmpty() )
	lnm.set( geomid.asInt() );

    setUiName( toUiString(lnm) );
    linename_->text()->setText( toUiString(lnm) );

    if ( scene_ )
    {
	setAnnotColor( scene_->getAnnotColor() );
	linename_->text()->setFontData( scene_->getAnnotFont(),
					getPixelDensity() );
    }

    geomidchanged_.trigger();
}


MultiID Seis2DDisplay::getMultiID() const
{
    MultiID linemultiid = SurvGeom2DTranslatorGroup::ioContext().getSelKey();
    linemultiid.setObjectID( geomid_.asInt() );
    return linemultiid;
}


BufferString Seis2DDisplay::getLineName() const
{
    if ( !Survey::GM().getName(geomid_) )
	return BufferString( getName() );

    return Survey::GM().getName(geomid_);
}


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry )
{
    geometry_ = geometry;
    const TypeSet<PosInfo::Line2DPos>& linepositions = geometry.positions();
    const int tracestep = geometry.trcNrRange().step_;

    const int possz = linepositions.size();
    trcdisplayinfo_.alltrcnrs_.erase();
    trcdisplayinfo_.alltrcpos_.erase();
    trcdisplayinfo_.alltrcnrs_.setCapacity( possz, false );
    trcdisplayinfo_.alltrcpos_.setCapacity( possz, false );

    maxtrcnrrg_.set( 0, mUdf(int), tracestep );
    for ( int idx=0; idx<possz; idx++ )
    {
	if ( idx == 0 )
            maxtrcnrrg_.start_ = maxtrcnrrg_.stop_ = linepositions[idx].nr_;
	else
	    maxtrcnrrg_.include( linepositions[idx].nr_, false );
	trcdisplayinfo_.alltrcnrs_ += linepositions[idx].nr_;
	trcdisplayinfo_.alltrcpos_ += linepositions[idx].coord_;
    }

    trcdisplayinfo_.zrg_.step_ = datatransform_
                                 ? datatransform_->getZInterval( false ).step_
                                 : geometry_.zRange().step_;
    setTraceNrRange( maxtrcnrrg_ );
    setZRange( geometry_.zRange() );

    updateRanges( false, true );
    geomchanged_.trigger();
}


void Seis2DDisplay::getTraceKeyPath( TrcKeyPath& res,TypeSet<Coord>* ) const
{
    if ( trcdisplayinfo_.rg_.isUdf() )
	return;

    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
    {
        if ( trcdisplayinfo_.alltrcnrs_[idx] < trcdisplayinfo_.rg_.start_ )
	    continue;
        else if ( trcdisplayinfo_.alltrcnrs_[idx] > trcdisplayinfo_.rg_.stop_ )
	    break;

	res += TrcKey( geomid_, trcdisplayinfo_.alltrcnrs_[idx] );
    }
}


Interval<float> Seis2DDisplay::getDataTraceRange() const
{
    return getZRange( false, -1 );
}


ZSampling Seis2DDisplay::getMaxZRange( bool displayspace ) const
{
    if ( !datatransform_ || !displayspace )
	return geometry_.zRange();

    return ZSampling( datatransform_->getZInterval( false ),
                      geometry_.zRange().step_ );
}


void Seis2DDisplay::setZRange( const ZSampling& nzrg )
{
    if ( mIsUdf(geometry_.zRange().start_) )
	return;

    const ZSampling maxzrg = getMaxZRange( true );
    const Interval<float> zrg( mMAX(maxzrg.start_,nzrg.start_),
                               mMIN(maxzrg.stop_,nzrg.stop_) );
    const bool hasdata = !datapacks_.isEmpty() && datapacks_[0];
    if ( hasdata && trcdisplayinfo_.zrg_.isEqual(zrg,mDefEps) )
	return;

    trcdisplayinfo_.zrg_.setInterval( zrg );

    updatePanelStripZRange();
    updatePanelStripPath();
    geomchanged_.trigger();
}


ZSampling Seis2DDisplay::getZRange( bool displayspace, int attrib ) const
{
    const bool alreadytransformed = alreadyTransformed( attrib );
    if ( alreadytransformed )
	return trcdisplayinfo_.zrg_;

    if ( datatransform_ && !displayspace )
	return datatransform_->getZInterval( true );

    return trcdisplayinfo_.zrg_;
}


const Interval<int> Seis2DDisplay::getSampleRange() const
{
    const ZSampling maxzrg = getMaxZRange( true );
    const Interval<int> samplerg(
                maxzrg.nearestIndex(trcdisplayinfo_.zrg_.start_),
                maxzrg.nearestIndex(trcdisplayinfo_.zrg_.stop_) );
    return samplerg;
}


#define mRetErrGeo \
{ pErrMsg("Geometry not set"); return; }

void Seis2DDisplay::setTraceNrRange( const Interval<int>& trcrg )
{
    trcdisplayinfo_.rg_.setUdf();
    if ( maxtrcnrrg_.isRev() )
	mRetErrGeo;

    const Interval<int> rg( maxtrcnrrg_.limitValue(trcrg.start_),
                            maxtrcnrrg_.limitValue(trcrg.stop_) );
    if ( !rg.width() )
	return;

    int startidx=-1, stopidx=-1;
    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs_.size(); idx++ )
    {
        if ( trcdisplayinfo_.alltrcnrs_[idx]>=rg.start_ )
	{
	    startidx = idx;
	    break;
	}
    }

    for ( int idx=trcdisplayinfo_.alltrcnrs_.size()-1; idx>=0; idx-- )
    {
        if ( trcdisplayinfo_.alltrcnrs_[idx]<=rg.stop_ )
	{
	    stopidx = idx;
	    break;
	}
    }

    if ( startidx<0 || stopidx<0 )
	mRetErrGeo;

    trcdisplayinfo_.rg_.start_ = trcdisplayinfo_.alltrcnrs_[startidx];
    trcdisplayinfo_.rg_.stop_ = trcdisplayinfo_.alltrcnrs_[stopidx];
    trcdisplayinfo_.size_ = stopidx - startidx + 1;
    updatePanelStripPath();
}


Interval<int> Seis2DDisplay::getTraceNrRange() const
{
    return trcdisplayinfo_.rg_;
}


const StepInterval<int>& Seis2DDisplay::getMaxTraceNrRange() const
{ return maxtrcnrrg_; }


bool Seis2DDisplay::setDataPackID( int attrib, const DataPackID& dpid,
				   TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM( getDataPackMgrID() );
    RefMan<RegularSeisDataPack> regsdp = dpm.get<RegularSeisDataPack>( dpid );
    return setSeisDataPack( attrib, regsdp.ptr(), taskr );
}


bool Seis2DDisplay::setSeisDataPack( int attrib, RegularSeisDataPack* dp,
				     TaskRunner* taskr )
{
    RefMan<RegularSeisDataPack> regsdp = dp;
    if ( !regsdp || regsdp->isEmpty() )
    {
	channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr, taskr );
	return false;
    }

    datapacks_.replace( attrib, regsdp.ptr() );

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );

    panelstrip_->turnOn( true );
    return true;
}


ConstRefMan<RegularSeisDataPack> Seis2DDisplay::getDataPack(
						int attrib ) const
{
    return mSelf().getDataPack( attrib );
}


RefMan<RegularSeisDataPack> Seis2DDisplay::getDataPack( int attrib )
{
    if ( !datapacks_.validIdx(attrib) || !datapacks_[attrib] )
	return nullptr;

    return datapacks_[attrib];
}


DataPackID Seis2DDisplay::getDataPackID( int attrib ) const
{
    ConstRefMan<RegularSeisDataPack> dp = getDataPack( attrib );
    return dp ? dp->id() : DataPack::cNoID();
}


ConstRefMan<RegularSeisDataPack> Seis2DDisplay::getDisplayedDataPack(
							int attrib ) const
{
    return mSelf().getDisplayedDataPack( attrib );
}


RefMan<RegularSeisDataPack> Seis2DDisplay::getDisplayedDataPack( int attrib )
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	if ( !transformedpacks_.validIdx(attrib) || !transformedpacks_[attrib] )
	    return nullptr;

	return transformedpacks_[attrib];
    }

    return getDataPack( attrib );
}


DataPackID Seis2DDisplay::getDisplayedDataPackID( int attrib ) const
{
    ConstRefMan<RegularSeisDataPack> dp = getDisplayedDataPack( attrib );
    return dp ? dp->id() : DataPack::cNoID();
}


void Seis2DDisplay::updateTexOriginAndScale( int attrib,
					     const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isDefined() || tkzs.isEmpty() )
	return;

    const TraceDisplayInfo& tdi = trcdisplayinfo_;

    const Coord origin( (tkzs.zsamp_.start_-tdi.zrg_.start_) / tdi.zrg_.step_,
                        tkzs.hsamp_.trcRange().start_ - tdi.rg_.start_ );

    const Coord scale( tkzs.zsamp_.step_ / tdi.zrg_.step_,
                       tkzs.hsamp_.trcRange().step_ );

    channels_->setOrigin( attrib, origin*(resolution_+1) );
    channels_->setScale( attrib, scale );
}


void Seis2DDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    ConstRefMan<RegularSeisDataPack> regsdp = getDisplayedDataPack( attrib );
    if ( !regsdp )
	return;

    updateTexOriginAndScale( attrib, regsdp->sampling() );

    const int nrversions = regsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    int sz0=mUdf(int), sz1=mUdf(int);
    for ( int idx=0; idx<nrversions; idx++ )
    {
	const Array3D<float>& array = regsdp->data( idx );

	if ( !idx )
	{
	    sz0 = 1 + (array.info().getSize(1)-1) * (resolution_+1);
	    sz1 = 1 + (array.info().getSize(2)-1) * (resolution_+1);
	}

	const ValueSeries<float>* stor = !resolution_ ? array.getStorage() : 0;
	bool ownsstor = false;

	if ( !stor )
	{
	    stor = new MultiArrayValueSeries<float,float>(sz0*sz1);
	    ownsstor = true;
	}

	if ( !stor || !stor->isOK() )
	{
	    channels_->turnOn( false );
	    pErrMsg("Insufficient memory; cannot display the 2D seismics.");
	    if ( ownsstor )
		delete stor;
	    return;
	}

	if ( ownsstor )
	{
	    auto* myvsptr = const_cast< ValueSeries<float>* >( stor );
	    if ( resolution_ == 0 )
		array.getAll( *myvsptr );
	    else
	    {
		Array2DSlice<float> slice2d( array );
		slice2d.setDimMap( 0, 1 );
		slice2d.setDimMap( 1, 2 );
		slice2d.setPos( 0, 0 );
		slice2d.init();

		Array2DReSampler<float,float> resampler(
				slice2d, *myvsptr, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( taskr, resampler );
	    }
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedVSData( attrib, idx, stor,
			ownsstor ? OD::TakeOverPtr : OD::UsePtr, nullptr );
    }

    channels_->turnOn( true );
}


void Seis2DDisplay::createTransformedDataPack( int attrib, TaskRunner* taskr )
{
    ConstRefMan<RegularSeisDataPack> regsdp = getDataPack( attrib );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    RefMan<RegularSeisDataPack> transformed;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, attrib );
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    datatransform_->loadDataIfMissing( voiidx_, taskr );
	}

	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( regsdp.ptr() );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.setOutputZRange( tkzs.zsamp_ );
	transformer.execute();

	transformed = transformer.getOutput();
    }

    transformedpacks_.replace( attrib, transformed.ptr() );
}


void Seis2DDisplay::updatePanelStripPath()
{
    TraceDisplayInfo& tdi = trcdisplayinfo_;
    const TypeSet<int>& bends = geometry_.getBendPoints();
    tdi.alljoints_.erase();

    for ( int idx=0; idx<bends.size(); idx++ )
    {
        if ( tdi.alltrcnrs_[bends[idx]]>tdi.rg_.start_ &&
	    (!tdi.alljoints_.isEmpty()
             && tdi.alltrcnrs_[tdi.alljoints_.last()] < tdi.rg_.start_) )
	{
            const int trcidx = tdi.alltrcnrs_.indexOf( tdi.rg_.start_ );
	    if ( tdi.alltrcnrs_.validIdx(trcidx) )
		tdi.alljoints_ += trcidx;
	}

        if ( tdi.alltrcnrs_[bends[idx]]>tdi.rg_.stop_ &&
	    (!tdi.alljoints_.isEmpty()
             && tdi.alltrcnrs_[tdi.alljoints_.last()] < tdi.rg_.stop_) )
	{
            const int trcidx = tdi.alltrcnrs_.indexOf( tdi.rg_.stop_ );
	    if ( tdi.alltrcnrs_.validIdx(trcidx) )
		tdi.alljoints_ += trcidx;
	}

	tdi.alljoints_ += bends[idx];
    }

    TypeSet<int> knots;

    for ( int idx=0; idx<tdi.alljoints_.size(); idx++ )
    {
	const int trcidx = tdi.alljoints_[idx];
	if ( !tdi.rg_.includes(tdi.alltrcnrs_[trcidx],true) )
	    continue;

	if ( !knots.isEmpty() )
	{
	    for ( int posidx=tdi.alljoints_[idx-1]+1;
		  posidx<tdi.alljoints_[idx]; posidx++ )
	    {
		const Coord pos = tdi.alltrcpos_[posidx];
		double d0 = pos.distTo( tdi.alltrcpos_[posidx-1] );
		double d1 = pos.distTo( tdi.alltrcpos_[posidx+1] );
		d0 *= abs( tdi.alltrcnrs_[posidx+1]-tdi.alltrcnrs_[posidx] );
		d1 *= abs( tdi.alltrcnrs_[posidx-1]-tdi.alltrcnrs_[posidx] );
		if ( (d0+d1)>0.0 && fabs(d0-d1)/(d0+d1)>0.1 )
		    knots += posidx;
	    }
	}
	knots += tdi.alljoints_[idx];
    }

    TypeSet<Coord> path;
    TypeSet<float> mapping;
    path.setCapacity( knots.size(), false );
    mapping.setCapacity( knots.size(), false );

    polyline_->removeAllPoints();
    for ( int idx=0; idx<knots.size(); idx++ )
    {
	path += tdi.alltrcpos_[knots[idx]];
	const float diff =
                mCast(float,tdi.alltrcnrs_[knots[idx]]-tdi.rg_.start_);
	mapping += diff * (resolution_+1);

        const Coord3 linepos( path[idx], tdi.zrg_.start_ );
	polyline_->addPoint( linepos );
    }

    panelstrip_->setPath( path );
    panelstrip_->setPath2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float diff =
                mCast(float,updatestageinfo_.oldtrcrgstart_-tdi.rg_.start_);
	panelstrip_->setPathTextureShift( diff*(resolution_+1) );
    }

    updateLineNamePos();
}


void Seis2DDisplay::updatePanelStripZRange()
{
    panelstrip_->setZRange( trcdisplayinfo_.zrg_ );
    const Interval<float> mapping( 0.0f,
		mCast(float,trcdisplayinfo_.zrg_.nrSteps())*(resolution_+1) );
    panelstrip_->setZRange2TextureMapping( mapping );

    if ( getUpdateStageNr() )
    {
	const float diff =
                updatestageinfo_.oldzrgstart_ - trcdisplayinfo_.zrg_.start_;
        const float factor = (resolution_+1) / trcdisplayinfo_.zrg_.step_;
	panelstrip_->setZTextureShift( diff*factor );
    }

    updateLineNamePos();
}


void Seis2DDisplay::annotateNextUpdateStage( bool yn )
{
    if ( !yn )
    {
	panelstrip_->setPathTextureShift( 0.0 );
	panelstrip_->setZTextureShift( 0.0 );
    }
    else if ( !getUpdateStageNr() )
    {
        updatestageinfo_.oldtrcrgstart_ =mCast(float,trcdisplayinfo_.rg_.start_);
        updatestageinfo_.oldzrgstart_ = mCast(float,trcdisplayinfo_.zrg_.start_);
    }
    else
	panelstrip_->freezeDisplay( false );	// thaw to refreeze

    panelstrip_->freezeDisplay( yn );
    SurveyObject::annotateNextUpdateStage( yn );
}


void Seis2DDisplay::updateLineNamePos()
{
    const int trcidx =
            trcdisplayinfo_.alltrcnrs_.indexOf( trcdisplayinfo_.rg_.start_ );
    if ( trcidx < 0 )
	return;

    Coord3 pos( trcdisplayinfo_.alltrcpos_[trcidx],trcdisplayinfo_.zrg_.start_ );
    linename_->text()->setPosition( pos );
}


SurveyObject* Seis2DDisplay::duplicate( TaskRunner* taskr ) const
{
    auto* s2dd = new Seis2DDisplay;
    s2dd->setGeometry( geometry_ );
    s2dd->setZRange( trcdisplayinfo_.zrg_ );
    s2dd->setTraceNrRange( getTraceNrRange() );
    s2dd->setResolution( getResolution(), taskr );
    s2dd->setGeomID( geomid_ );
    s2dd->setZAxisTransform( datatransform_.getNonConstPtr(), taskr );
    s2dd->showPanel( panelstrip_->isOn() );

    while ( nrAttribs() > s2dd->nrAttribs() )
	s2dd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const TypeSet<Attrib::SelSpec>* selspecs = getSelSpecs( idx );
	if ( !selspecs )
	    continue;

	s2dd->setSelSpecs( idx, *selspecs );
	ConstRefMan<RegularSeisDataPack> regsdp = getDataPack( idx );
	s2dd->setSeisDataPack( idx, regsdp.getNonConstPtr(), taskr );
	const ColTab::MapperSetup* mappersetup = getColTabMapperSetup( idx );
	if ( mappersetup )
	    s2dd->setColTabMapperSetup( idx,*mappersetup,taskr );

	const ColTab::Sequence* colseq = getColTabSequence( idx );
	if ( colseq )
	    s2dd->setColTabSequence( idx, *colseq, taskr );
    }

    return s2dd;
}


float Seis2DDisplay::calcDist( const Coord3& pos ) const
{
    Coord3 xytpos;
    mVisTrans::transformBack( scene_ ? scene_->getUTM2DisplayTransform()
				     : nullptr, pos, xytpos );

    int trcidx; float minsqdist;
    getNearestTrace( xytpos, trcidx, minsqdist );
    if ( minsqdist<0 || mIsUdf(minsqdist) )
	return mUdf(float);

    ZSampling zrg = getZRange( true );
    float zdif = 0;
    if ( !zrg.includes(xytpos.z_,false) )
    {
        zdif = (float) mMIN(fabs(xytpos.z_-zrg.start_), fabs(xytpos.z_-zrg.stop_));
	const float zscale = scene_
	    ? scene_->getZScale() * scene_->getFixedZStretch()
	    : SI().zScale();
	zdif *= zscale;
    }

    return Math::Sqrt( minsqdist + zdif*zdif );
}


void Seis2DDisplay::setDisplayTransformation( const mVisTrans* tf )
{
    transformation_ = tf;
    polyline_->setDisplayTransformation( transformation_.ptr() );
    panelstrip_->setDisplayTransformation( transformation_.ptr() );
    linename_->setDisplayTransformation( transformation_.ptr() );
}


const mVisTrans* Seis2DDisplay::getDisplayTransformation() const
{
    return transformation_.ptr();
}


void Seis2DDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );
    pixeldensity_ = dpi;
    if ( linename_ )
	linename_->text()->updateFontSize( dpi );
}


void Seis2DDisplay::showLineName( bool yn )
{
    if ( !linename_ )
	return;

    linename_->turnOn( yn );
    updateLineNamePos();
}


bool Seis2DDisplay::isLineNameShown() const
{ return linename_->isOn(); }


int Seis2DDisplay::nrResolutions() const
{ return 3; }


void Seis2DDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, taskr );

    updatePanelStripPath();
    updatePanelStripZRange();
}


TrcKeyZSampling Seis2DDisplay::getTrcKeyZSampling( bool displayspace,
						   int attrib ) const
{
    TrcKeyZSampling tkzs;
    tkzs.hsamp_.init( getGeomID() );
    const StepInterval<int> trcrg( getTraceNrRange() );
    tkzs.hsamp_.setTrcRange( trcrg );
    tkzs.zsamp_ = getZRange( displayspace, attrib );
    return tkzs;
}


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat( int ) const
{
    return SurveyObject::Cube;
}


void Seis2DDisplay::addCache()
{
    datapacks_ += nullptr;
    transformedpacks_ += nullptr;
}


void Seis2DDisplay::removeCache( int attrib )
{
    datapacks_.removeSingle( attrib );
    transformedpacks_.removeSingle( attrib );
}


void Seis2DDisplay::swapCache( int a0, int a1 )
{
    datapacks_.swap( a0, a1 );
    transformedpacks_.swap( a0, a1 );
}


void Seis2DDisplay::emptyCache( int attrib )
{
    datapacks_.replace( attrib, nullptr );
    transformedpacks_.replace( attrib, nullptr );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr, nullptr );
}


bool Seis2DDisplay::hasCache( int attrib ) const
{
    return datapacks_[attrib];
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo& evinfo,
				     IOPar& par ) const
{
    par.setEmpty();
    SurveyObject::getMousePosInfo( evinfo, par );
    par.set( sKey::XCoord(), evinfo.worldpickedpos.x_ );
    par.set( sKey::YCoord(), evinfo.worldpickedpos.y_ );
    par.set( sKey::LineKey(), name() );

    int dataidx = -1;
    float minsqdist;
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,minsqdist) )
	par.set( sKey::TraceKey(),
		TrcKey(geomid_,geometry_.positions()[dataidx].nr_) );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     uiString& info ) const
{
    getObjectInfo( info );
    getValueString( pos, val );

    int dataidx = -1;
    float minsqdist;
    if ( getNearestTrace(pos,dataidx,minsqdist) )
    {
	info.append( toUiString(", TrcNr: %1")
			.arg(geometry_.positions()[dataidx].nr_) );

	const Survey::Geometry* geom = Survey::GM().getGeometry(geomid_);
	const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
	if ( geom2d && geom2d->spnrs().validIdx(dataidx) )
	    info.append( toUiString(", SP: %1")
					    .arg(geom2d->spnrs()[dataidx]) );
    }
}


void Seis2DDisplay::getObjectInfo( uiString& info ) const
{
    info = uiStrings::sLine();
    info.appendPhrase( toUiString(getLineName()), uiString::MoreInfo,
					uiString::OnSameLine );
}


bool Seis2DDisplay::getCacheValue( int attrib, int version,
				    const Coord3& pos, float& res ) const
{
    ConstRefMan<RegularSeisDataPack> regsdp = getDisplayedDataPack( attrib );
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    int traceidx = -1;
    float minsqdist;
    if ( !getNearestTrace(pos,traceidx,minsqdist) )
	return false;

    const int trcnr = geometry_.positions()[traceidx].nr_;
    const TrcKey trckey( geomid_, trcnr );
    const int trcidx = regsdp->getNearestGlobalIdx( trckey );
    const int sampidx = regsdp->zRange().nearestIndex( pos.z_ );
    const Array3DImpl<float>& array = regsdp->data( version );
    if ( !array.info().validPos(0,trcidx,sampidx) )
	return false;

    res = array.get( 0, trcidx, sampidx );
    return true;
}


int Seis2DDisplay::getNearestTraceNr( const Coord3& pos ) const
{
    int trcidx = -1;
    float minsqdist;
    if ( getNearestTrace(pos,trcidx,minsqdist) )
	return geometry_.positions()[trcidx].nr_;

    return mUdf(int);
}


Coord3 Seis2DDisplay::getNearestSubPos( const Coord3& pos,
					bool usemaxrange ) const
{
    int trcnr1st, trcnr2nd;
    float frac;
    if ( getNearestSegment(pos, usemaxrange, trcnr1st, trcnr2nd, frac) < 0.0 )
	return Coord3::udf();

    const Coord subpos = getCoord(trcnr1st)*(1-frac) + getCoord(trcnr2nd)*frac;
    const Interval<float> zrg = usemaxrange ? getMaxZRange(false) :
					      getZRange(false);
    return Coord3( subpos, zrg.limitValue(pos.z_) );
}


float Seis2DDisplay::getNearestSegment( const Coord3& pos, bool usemaxrange,
					int& trcnr1st, int& trcnr2nd,
					float& frac ) const
{
    float mindist2 = MAXFLOAT;
    Interval<int> trcrg = getTraceNrRange();
    if ( usemaxrange )
	trcrg = getMaxTraceNrRange();

    const TypeSet<PosInfo::Line2DPos>& posns = geometry_.positions();
    for ( int aidx=0; aidx<posns.size()-1; aidx++ )
    {
	const Coord posa = posns[aidx].coord_;
	if ( !posa.isDefined() || !trcrg.includes(posns[aidx].nr_,false) )
	    continue;

	Coord posb = Coord::udf();
	int bidx = aidx;

	while ( !posb.isDefined() && bidx<posns.size()-1 &&
		trcrg.includes(posns[bidx+1].nr_,false) )
	{
	    bidx++;
	    posb = posns[bidx].coord_;
	}

	if ( !posb.isDefined() )
	{
	    bidx = aidx;
	    posb = posa;
	}

	const float dist2a = (float) posa.sqDistTo( pos );
	const float dist2b = (float) posb.sqDistTo( pos );
	const float dist2c = (float) posa.sqDistTo( posb );

	if ( dist2b >= dist2a+dist2c )
	{
	    if ( mindist2 > dist2a )
	    {
		mindist2 = dist2a;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 0.0;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 1.0;
	    }
	    continue;
	}

	const float dista = Math::Sqrt( dist2a );
	const float distb = Math::Sqrt( dist2b );
	const float distc = Math::Sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    trcnr1st = posns[aidx].nr_;
	    trcnr2nd = posns[bidx].nr_;
	    frac = Math::Sqrt( dist2a - height2 ) / distc;
	}
    }

    return mindist2!=MAXFLOAT ? Math::Sqrt(mindist2) : -1.0f;
}


void Seis2DDisplay::snapToTracePos( Coord3& pos ) const
{
    int trcidx = -1;
    float minsqdist;
    getNearestTrace( pos, trcidx, minsqdist );

    if ( trcidx<0 )
	return;

    const Coord& crd = geometry_.positions()[trcidx].coord_;
    pos.x_ = crd.x_; pos.y_ = crd.y_;
}


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& minsqdist ) const
{
    if ( geometry_.isEmpty() )
	return false;

    const int nidx = geometry_.nearestIdx( pos, trcdisplayinfo_.rg_ );
    if ( nidx >= 0 )
	minsqdist = (float) geometry_.positions()[nidx].coord_.sqDistTo( pos );

    trcidx = nidx;
    return trcidx >= 0;
}


Coord Seis2DDisplay::getCoord( int trcnr ) const
{
    const int sz = geometry_.positions().size();
    if ( !sz )
	return Coord::udf();

    if ( prevtrcidx_ >= sz )
	prevtrcidx_ = sz-1;

    const int prevnr = geometry_.positions()[prevtrcidx_].nr_;
    int dir = (geometry_.positions()[0].nr_-prevnr)*(trcnr-prevnr)<0 ? 1 : -1;

    for ( int cnt=0; cnt<=1; cnt++ )
    {
	for ( int idx=prevtrcidx_+cnt*dir; idx>=0 && idx<sz; idx+=dir )
	{
	    if ( geometry_.positions()[idx].nr_ == trcnr )
	    {
		prevtrcidx_ = idx;
		return geometry_.positions()[idx].coord_;
	    }
	}
	dir = -dir;
    }
    return Coord::udf();
}


Coord Seis2DDisplay::getNormal( int trcnr ) const
{
    return geometry_.getNormal( trcnr );
}


bool Seis2DDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* taskr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( voiidx_!=-1 )
	    datatransform_->removeVolumeOfInterest( voiidx_ );

	if ( datatransform_->changeNotifier() )
	    mDetachCB( *datatransform_->changeNotifier(),
		       Seis2DDisplay::dataTransformCB );
    }

    datatransform_ = zat;
    voiidx_ = -1;

    if ( datatransform_ )
    {
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    mAttachCB( *datatransform_->changeNotifier(),
		       Seis2DDisplay::dataTransformCB );
    }

    return true;
}


const ZAxisTransform* Seis2DDisplay::getZAxisTransform() const
{
    return datatransform_.ptr();
}


void Seis2DDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	createTransformedDataPack( idx, nullptr );
	updateChannels( idx, nullptr );
    }
}


void Seis2DDisplay::updateRanges( bool updatetrc, bool updatez )
{
    // TODO: handle update trcrg
    if ( updatez && datatransform_ )
	setZRange( datatransform_->getZInterval( false ) );
}


void Seis2DDisplay::clearTexture( int attribnr )
{
    if ( channels_->nrChannels() > 1 )
    {
	Attrib::SelSpec as;
	as.set2DFlag(true);
	setSelSpec( attribnr, as );
	channels_->unfreezeOldData( attribnr );
	enableAttrib( attribnr, false );
	return;
    }

    channels_->setNrVersions( attribnr, 1 );
    channels_->setUnMappedVSData( attribnr, 0, 0, OD::UsePtr, nullptr );
    channels_->unfreezeOldData( attribnr );
    channels_->turnOn( false );

    Attrib::SelSpec as;
    as.set2DFlag(true);
    setSelSpec( attribnr, as );
}


void Seis2DDisplay::setAnnotColor( OD::Color col )
{
    linename_->getMaterial()->setColor( col ); // not sure if this has to be set
    linename_->text()->setColor( col );
}


OD::Color Seis2DDisplay::getAnnotColor() const
{
    return linename_->getMaterial()->getColor();
}


const visBase::TexturePanelStrip* Seis2DDisplay::getTexturePanelStrip() const
{
    return panelstrip_.ptr();
}


visBase::TexturePanelStrip* Seis2DDisplay::getTexturePanelStrip()
{
    return panelstrip_.ptr();
}


const visBase::Text2* Seis2DDisplay::getVisTextLineName() const
{
    return linename_.ptr();
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const MultiID& lineset,
						const char* linenmptr )
{
    StringView linenm = linenmptr;
    TypeSet<VisID> ids;
    visBase::DM().getIDs( typeid(Seis2DDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && !linenm.isEmpty() && linenm==s2dd->getLineName() )
	    return s2dd;
    }

    return nullptr;
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const Pos::GeomID& geomid )
{
    TypeSet<VisID> ids;
    visBase::DM().getIDs( typeid(Seis2DDisplay), ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && s2dd->getGeomID()==geomid )
	    return s2dd;
    }

    return nullptr;
}


Coord3 Seis2DDisplay::projectOnNearestPanel( const Coord3& pos,
					     int* nearestpanelidxptr )
{
    float mindist2 = MAXFLOAT;
    int nearestpanelidx = -1;
    Coord projpos = Coord::udf();

    const TraceDisplayInfo& tdi = trcdisplayinfo_;
    for ( int idx=0; idx<tdi.alljoints_.size()-1; idx++ )
    {
	const Coord posa = tdi.alltrcpos_[tdi.alljoints_[idx]];
	const Coord posb = tdi.alltrcpos_[tdi.alljoints_[idx+1]];

	const float dist2a = (float) posa.sqDistTo( pos );
	const float dist2b = (float) posb.sqDistTo( pos );
	const float dist2c = (float) posa.sqDistTo( posb );

	if ( dist2b >= dist2a+dist2c )
	{
	    if ( mindist2 > dist2a )
	    {
		mindist2 = dist2a;
		nearestpanelidx = idx;
		projpos = posa;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		nearestpanelidx = idx;
		projpos = posb;
	    }
	    continue;
	}

	const float dista = Math::Sqrt( dist2a );
	const float distb = Math::Sqrt( dist2b );
	const float distc = Math::Sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    nearestpanelidx = idx;
	    const float frac = Math::Sqrt( dist2a - height2 ) / distc;
	    projpos = posa*(1.0-frac) + posb*frac;
	}
    }

    if ( nearestpanelidxptr )
	*nearestpanelidxptr = nearestpanelidx;

    return Coord3( projpos, pos.z_ );
}


void Seis2DDisplay::getLineSegmentProjection( const Coord3 pos1,
					      const Coord3 pos2,
					      TypeSet<Coord3>& projcoords )
{
    const TraceDisplayInfo& tdi = trcdisplayinfo_;
    int panelidx1, panelidx2;
    const Coord3 projpos1 = projectOnNearestPanel( pos1, &panelidx1 );
    const Coord3 projpos2 = projectOnNearestPanel( pos2, &panelidx2 );

    projcoords.erase();
    if ( !projpos1.isDefined() || !projpos2.isDefined() )
	return;

    projcoords += projpos1;
    TypeSet<float> arclen;
    arclen += 0.0;

    for ( int cnt=abs(panelidx2-panelidx1); cnt>=0; cnt-- )
    {
	const int idx = panelidx2 + (panelidx1<panelidx2 ? 1-cnt : cnt );
	const Coord pos = cnt ? tdi.alltrcpos_[tdi.alljoints_[idx]] : projpos2;

	const float dist = mCast( float, pos.distTo(projcoords.last()) );
	if ( dist > mDefEps )
	{
	    projcoords += Coord3( pos, 0.0 );
	    arclen += arclen.last() + dist;
	}
    }

    for ( int idx=0; idx<projcoords.size(); idx++ )
    {
	const float totalarclen = arclen.last();
	const float frac = totalarclen ? arclen[idx]/totalarclen : 0.5f;
        projcoords[idx].z_ = projpos1.z_*(1.0-frac) + projpos2.z_*frac;
    }
}


void Seis2DDisplay::fillPar( IOPar& par ) const
{
    MultiTextureSurveyObject::fillPar( par );

    par.set( sKey::GeomID(), geomid_ );
    par.setYN( sKeyShowLineName(), isLineNameShown() );
    par.setYN( sKeyShowPanel(), isPanelShown() );
    par.setYN( sKeyShowPolyLine(), isPolyLineShown() );
    if ( !trcdisplayinfo_.alltrcnrs_.isEmpty() )
	par.set( sKeyTrcNrRange(), trcdisplayinfo_.rg_ );

    par.set( sKeyZRange(), trcdisplayinfo_.zrg_ );
}


bool Seis2DDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar(par) )
	return false;

    Interval<int> trcnrrg;
    if ( par.get(sKeyTrcNrRange(),trcnrrg) )
	trcdisplayinfo_.rg_ = trcnrrg;

    bool doshow = false;
    par.getYN( sKeyShowLineName(), doshow );
    showLineName( doshow );

    if ( par.getYN(sKeyShowPanel(),doshow) )
	showPanel( doshow );

    if ( par.getYN(sKeyShowPolyLine(),doshow) )
	showPolyLine( doshow );

    par.get( sKeyZRange(), trcdisplayinfo_.zrg_ );

    Pos::GeomID geomid;
    if ( par.get(sKey::GeomID(),geomid) )
	setGeomID( geomid );

    return true;
}

} // namespace visSurvey
