/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismultiattribsurvobj.h"

#include "attribsel.h"
#include "bindatadesc.h"
#include "color.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "settingsaccess.h"
#include "zaxistransform.h"

#include "vismaterial.h"
#include "vistexturechannel2rgba.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistexturechannels.h"


namespace visSurvey {

const char* MultiTextureSurveyObject::sKeyResolution()	{ return "Resolution"; }
const char* MultiTextureSurveyObject::sKeyTextTrans()	{ return "Trans"; }
const char* MultiTextureSurveyObject::sKeySequence()	{ return "Sequence"; }
const char* MultiTextureSurveyObject::sKeyMapper()	{ return "Mapper"; }


MultiTextureSurveyObject::MultiTextureSurveyObject()
    : VisualObjectImpl(true)
    , channels_( visBase::TextureChannels::create() )
    , onoffstatus_( true )
    , resolution_( 0 )
    , enabletextureinterp_( true )
{
    channels_->ref();
    channels_->setChannels2RGBA( visBase::ColTabTextureChannel2RGBA::create() );

    channels_->enableTextureInterpolation( enabletextureinterp_ );

    getMaterial()->setColor( OD::Color::White() );
    float ambience = 0.8f;
    float diffuseintensity = 0.8f;
    Settings::common().get( SettingsAccess::sKeyDefaultAmbientReflectivity(),
			    ambience );
    Settings::common().get( SettingsAccess::sKeyDefaultDiffuseReflectivity(),
			    diffuseintensity );
    material_->setAmbience( ambience );
    material_->setDiffIntensity( diffuseintensity );
}


MultiTextureSurveyObject::~MultiTextureSurveyObject()
{
    deepErase( as_ );
    setZAxisTransform( 0, 0 );
    channels_->unRef();

    deepErase( userrefs_ );
}


bool MultiTextureSurveyObject::init()
{
    resolution_ = SettingsAccess().getDefaultTexResFactor( nrResolutions() );
    return addAttrib();
}


void MultiTextureSurveyObject::allowShading( bool yn )
{
    if ( channels_ && channels_->getChannels2RGBA() )
	channels_->getChannels2RGBA()->allowShading( yn );
}


bool MultiTextureSurveyObject::turnOn( bool yn )
{
    const bool res = isOn();
    onoffstatus_ = yn;
    updateMainSwitch();
    return res;
}


bool MultiTextureSurveyObject::isOn() const
{ return onoffstatus_; }


int MultiTextureSurveyObject::getResolution() const
{ return resolution_; }


bool
MultiTextureSurveyObject::setChannels2RGBA( visBase::TextureChannel2RGBA* t )
{
    RefMan<visBase::TextureChannel2RGBA> dummy( t );
    if ( !channels_ ) return true;

    return channels_->setChannels2RGBA(t);
}


visBase::TextureChannel2RGBA* MultiTextureSurveyObject::getChannels2RGBA()
{
    return channels_ ? channels_->getChannels2RGBA() : 0;
}


void MultiTextureSurveyObject::updateMainSwitch()
{
    if ( onoffstatus_ && !hasSingleColorFallback() )
	VisualObjectImpl::turnOn( isAnyAttribEnabled() );
    else
	VisualObjectImpl::turnOn( onoffstatus_ );
}


bool MultiTextureSurveyObject::isShown() const
{ return VisualObjectImpl::isOn(); }


bool MultiTextureSurveyObject::canHaveMultipleAttribs() const
{ return true; }


int MultiTextureSurveyObject::nrAttribs() const
{ return as_.size(); }


bool MultiTextureSurveyObject::canAddAttrib( int nr ) const
{
    const int maxnr = channels_->getChannels2RGBA()->maxNrChannels();

    if ( !maxnr ) return true;

    return nrAttribs()+nr<=maxnr;
}


bool MultiTextureSurveyObject::canRemoveAttrib() const
{
    const int newnrattribs = nrAttribs()-1;
    if ( newnrattribs<channels_->getChannels2RGBA()->minNrChannels() )
	return false;

    return true;
}


bool MultiTextureSurveyObject::addAttrib()
{
    BufferStringSet* aatrnms = new BufferStringSet();
    aatrnms->allowNull();
    userrefs_ += aatrnms;
    Attrib::SelSpec as;
    if ( getAllowedDataType() == OD::Only2D )
    {
	as.set2DFlag( true );
	as.setObjectRef( getMultiID().toString() );
    }

    as_ += new TypeSet<Attrib::SelSpec>( 1, as );
    addCache();

    while ( channels_->nrChannels()<as_.size() )
	channels_->addChannel();

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::removeAttrib( int attrib )
{
    if ( as_.size()<2 || !as_.validIdx(attrib) )
	return false;

    channels_->removeChannel( attrib );

    delete as_[attrib];
    as_.removeSingle( attrib );
    delete userrefs_.removeSingle( attrib );

    removeCache( attrib );

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::swapAttribs( int a0, int a1 )
{
    if ( a0<0 || a1<0 || a0>=as_.size() || a1>=as_.size() )
	return false;

    channels_->swapChannels( a0, a1 );
    as_.swap( a0, a1 );
    userrefs_.swap( a0, a1 );
    swapCache( a0, a1 );

    return true;
}


void MultiTextureSurveyObject::clearTextures()
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	Attrib::SelSpec as; setSelSpec( idx, as );

	for ( int idy=nrTextures(idx)-1; idy>=0; idy-- )
	    channels_->setUnMappedData( idx, idy, 0, OD::UsePtr, 0 );
    }
}


void MultiTextureSurveyObject::setAttribTransparency( int attrib,
						      unsigned char nt )
{
    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
		     channels_->getChannels2RGBA() );
    if ( cttc2rgba )
	cttc2rgba->setTransparency( attrib, nt );

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgbatc2rgba,
		     channels_->getChannels2RGBA() );
    if ( rgbatc2rgba )
	rgbatc2rgba->setTransparency( nt );
}



unsigned char MultiTextureSurveyObject::getAttribTransparency(int attrib) const
{
    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
		     channels_->getChannels2RGBA() );
    if ( cttc2rgba )
	return cttc2rgba->getTransparency( attrib );

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgbatc2rgba,
		     channels_->getChannels2RGBA() );
    if ( rgbatc2rgba )
	return rgbatc2rgba->getTransparency();

    return 0;
}


const TypeSet<Attrib::SelSpec>*
	MultiTextureSurveyObject::getSelSpecs( int attrib ) const
{
    return as_.validIdx(attrib) ? as_[attrib] : 0;
}


const Attrib::SelSpec* MultiTextureSurveyObject::getSelSpec(
						int attrib, int version ) const
{
    if ( !as_.validIdx(attrib) )
	return nullptr;

    if ( !as_[attrib]->validIdx(version) )
	version = channels_->currentVersion( attrib );

    return as_[attrib]->validIdx(version) ? &(*as_[attrib])[version] : nullptr;
}


void MultiTextureSurveyObject::setSelSpec( int attrib,
					   const Attrib::SelSpec& as )
{
    setSelSpecs( attrib, TypeSet<Attrib::SelSpec>(1,as) );
}


void MultiTextureSurveyObject::setSelSpecs( int attrib,
					const TypeSet<Attrib::SelSpec>& as )
{
    SurveyObject::setSelSpecs( attrib, as );

    if ( !as_.validIdx(attrib) )
	return;

    *as_[attrib] = as;

    emptyCache( attrib );

    BufferStringSet* attrnms = new BufferStringSet();
    for ( int idx=0; idx<as.size(); idx++ )
	attrnms->add( as[idx].userRef() );
    delete userrefs_.replace( attrib, attrnms );

    channels_->setNrVersions( attrib, as.size() );
    if ( !as.isEmpty() && StringView(as[0].userRef()).isEmpty() )
	channels_->getChannels2RGBA()->setEnabled( attrib, true );
}


bool MultiTextureSurveyObject::textureInterpolationEnabled() const
{ return enabletextureinterp_; }


void MultiTextureSurveyObject::enableTextureInterpolation( bool yn )
{
    if ( enabletextureinterp_==yn )
	return;

    enabletextureinterp_ = yn;

    if ( channels_ )
	channels_->enableTextureInterpolation( yn );
}


bool MultiTextureSurveyObject::isAngle( int attrib ) const
{
    return false;
}


void MultiTextureSurveyObject::setAngleFlag( int attrib, bool yn )
{
}


bool MultiTextureSurveyObject::isAttribEnabled( int attrib ) const
{
    return channels_->getChannels2RGBA()->isEnabled( attrib );
}


void MultiTextureSurveyObject::enableAttrib( int attrib, bool yn )
{
    channels_->getChannels2RGBA()->setEnabled( attrib, yn );
    updateMainSwitch();
}


bool MultiTextureSurveyObject::canDisplayInteractively() const
{
    return canDisplayInteractively( Pos::GeomID::udf() );
}


bool MultiTextureSurveyObject::canDisplayInteractively(
						    Pos::GeomID geomid ) const
{
    if ( !channels_->getChannels2RGBA() ||
			    !channels_->getChannels2RGBA()->usesShading() )
	return false;

    for ( int attrib=0; attrib<nrAttribs(); attrib++ )
	if ( !getSelSpec(attrib)->getPreloadDataDesc(geomid) )
	    return false;

    return true;;
}


const TypeSet<float>*
MultiTextureSurveyObject::getHistogram( int attrib ) const
{
    return channels_->getHistogram( attrib );
}


int MultiTextureSurveyObject::getColTabID( int attrib ) const
{
    return -1;
}


void MultiTextureSurveyObject::setColTabSequence( int attrib,
			      ColTab::Sequence const& seq, TaskRunner* )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    channels_->getChannels2RGBA()->setSequence( attrib, seq );
}


bool MultiTextureSurveyObject::canSetColTabSequence() const
{
    return channels_->getChannels2RGBA()->canSetSequence();
}



const ColTab::Sequence*
MultiTextureSurveyObject::getColTabSequence( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return 0;

    return channels_->getChannels2RGBA()->getSequence( attrib );
}


void MultiTextureSurveyObject::setColTabMapperSetup( int attrib,
			      const ColTab::MapperSetup& mapper, TaskRunner* )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    const ColTab::MapperSetup& old = channels_->getColTabMapperSetup(attrib,0);
    if ( old!=mapper )
    {
	const bool needsreclip = old.needsReClip( mapper );
	channels_->setColTabMapperSetup( attrib, mapper );
	channels_->reMapData( attrib, !needsreclip, 0 );
    }
}


const ColTab::MapperSetup*
MultiTextureSurveyObject::getColTabMapperSetup( int attrib ) const
{
    return getColTabMapperSetup( attrib, mUdf(int) );
}


const ColTab::MapperSetup*
MultiTextureSurveyObject::getColTabMapperSetup( int attrib, int version ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return 0;

    if ( mIsUdf(version) || version<0
	    		 || version >= channels_->nrVersions(attrib) )
	version = channels_->currentVersion( attrib );

    return &channels_->getColTabMapperSetup( attrib, version );
}


int MultiTextureSurveyObject::nrTextures( int attrib ) const
{
    return channels_->nrVersions( attrib );
}


void MultiTextureSurveyObject::selectTexture( int attrib, int idx )
{
    if ( !as_.validIdx(attrib) || idx<0 )
	return;

    if ( idx>=channels_->nrVersions(attrib) )
	return;

    channels_->setCurrentVersion( attrib, idx );
}


int MultiTextureSurveyObject::selectedTexture( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() ) return 0;

    return channels_->currentVersion( attrib );
}


void MultiTextureSurveyObject::fillPar( IOPar& par ) const
{
    visSurvey::SurveyObject::fillPar( par );
    visBase::VisualObjectImpl::fillPar( par );
    par.set( sKeyResolution(), resolution_ );
    par.setYN( visBase::VisualObjectImpl::sKeyIsOn(), isOn() );
    for ( int attrib=nrAttribs()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	getSelSpec(attrib)->fillPar( attribpar );

	if ( canSetColTabSequence() && getColTabSequence( attrib ) )
	{
	    IOPar seqpar;
	    const ColTab::Sequence* seq = getColTabSequence( attrib );
	    if ( seq->isSys() )
		seqpar.set( sKey::Name(), seq->name() );
	    else
		seq->fillPar( seqpar );

	    attribpar.mergeComp( seqpar, sKeySequence() );
	}

	if ( getColTabMapperSetup( attrib ) )
	{
	    IOPar mapperpar;
	    getColTabMapperSetup( attrib )->fillPar( mapperpar );
	    attribpar.mergeComp( mapperpar, sKeyMapper() );
	}

	attribpar.set( sKeyTextTrans(),
		       getAttribTransparency( attrib ) );
	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 isAttribEnabled( attrib ) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), nrAttribs() );
}


bool MultiTextureSurveyObject::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

    par.get( sKeyResolution(), resolution_ );

    bool ison = true;
    par.getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
    turnOn( ison );

    return true;
}


void MultiTextureSurveyObject::getValueString( const Coord3& pos,
						BufferString& val ) const
{
    val = "undef";
    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, ctab,
	    channels_ ? channels_->getChannels2RGBA() : 0 );
    if ( !ctab || !pos.isDefined() )
	return;

    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	if ( !isAttribEnabled(idx) || ctab->getTransparency(idx)==255 )
	    continue;

	const int version = channels_->currentVersion( idx );

	float fval;
	if ( !getCacheValue(idx, version, pos, fval ) )
	    continue;

	if ( !Math::IsNormalNumber(fval) )
	    fval = mUdf(float);

	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !hasCache(idy) || !isAttribEnabled(idy) ||
		 ctab->getTransparency(idy)==255 )
		continue;

	    islowest = false;
	    break;
	}

	if ( !islowest )
	{
	    const ColTab::Sequence* seq = ctab->getSequence( idx );
	    const ColTab::Mapper& map = channels_->getColTabMapper(idx,version);

	    const OD::Color col = mIsUdf(fval) ? seq->undefColor()
					   : seq->color( map.position(fval) );
	    if ( col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	    val = fval;

	if ( nrAttribs()>1 )
	{
	    BufferString attribstr = "(";
	    attribstr += getSelSpec(idx)->userRef();
	    attribstr += ")";
	    val.replaceAt( cValNameOffset(), (const char*)attribstr);
	}

	return;
    }
}


} // namespace visSurvey
