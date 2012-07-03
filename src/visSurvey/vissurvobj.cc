/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID mUnusedVar = "$Id: vissurvobj.cc,v 1.67 2012-07-03 08:41:52 cvskris Exp $";

#include "vissurvobj.h"

#include "attribsel.h"
#include "basemap.h"
#include "survinfo.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "vistexturechannel2rgba.h"
#include "visobject.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisbuf.h"

namespace visSurvey {


SurveyObject::SurveyObject()
    : scene_(0)
    , inlcrlsystem_( 0 )
    , basemapobj_(0)
    , locked_(false)
{
    setInlCrlSystem( SI().get3DGeometry(true) );
}

    
SurveyObject::~SurveyObject()
{
    deepErase(userrefs_);
    setInlCrlSystem( 0 );
}

float SurveyObject::sDefMaxDist()	{ return 10; }


SurveyObject::AttribFormat SurveyObject::getAttributeFormat( int attrib ) const
{ return SurveyObject::None; }

int SurveyObject::nrAttribs() const
{ return getAttributeFormat()==SurveyObject::None ? 0 : 1; }

bool SurveyObject::canAddAttrib(int) const
{ return canHaveMultipleAttribs(); }

bool SurveyObject::canRemoveAttrib() const
{ return canHaveMultipleAttribs() && nrAttribs()>1; }


void SurveyObject::setColTabMapperSetup( int, const ColTab::MapperSetup&,
					 TaskRunner*)
{}


bool SurveyObject::canHandleColTabSeqTrans(int) const
{ return true; }


const ColTab::MapperSetup* SurveyObject::getColTabMapperSetup( int, int ) const
{ return 0; }


void SurveyObject::setColTabSequence( int, const ColTab::Sequence&, TaskRunner*)
{}


void SurveyObject::setTraceData( int, SeisTrcBuf& trcs, TaskRunner* )
{ trcs.deepErase(); }


BufferString SurveyObject::getResolutionName( int res ) const
{
    if ( res == 0 ) return "Standard";
    else if ( res == 1 ) return "Higher";
    else if ( res == 2 ) return "Highest";
    else return "?";
}


void SurveyObject::setBaseMap( BaseMap* bm )
{
    if ( basemapobj_ )
    {
	if ( scene_ && scene_->getBaseMap() )
	    scene_->getBaseMap()->removeObject( basemapobj_ );

	delete basemapobj_;
	basemapobj_ = 0;
    }

    if ( bm )
    {
	basemapobj_ = createBaseMapObject();
	if ( basemapobj_ )
	    bm->addObject( basemapobj_ );
    }
}


void SurveyObject::setScene( Scene* sc )
{
    setBaseMap( sc ? sc->getBaseMap() : 0 );
    scene_ = sc;
}


bool SurveyObject::alreadyTransformed( int attrib ) const
{
    const Attrib::SelSpec* as = getSelSpec( attrib );
    if ( !as ) return false;

    const FixedString zdomainkey = as->zDomainKey();
    return scene_ && zdomainkey == scene_->zDomainKey();
}


void SurveyObject::getLineWidthBounds( int& min, int& max )
{ min = mUdf(int); max= mUdf(int); }


void SurveyObject::setInlCrlSystem(const InlCrlSystem* ics)
{
    if ( inlcrlsystem_ ) inlcrlsystem_->unRef();
    
    inlcrlsystem_ = ics;
    
    if ( inlcrlsystem_ ) inlcrlsystem_->ref();
}


const char* SurveyObject::getInlCrlSystemName() const
{
    return inlcrlsystem_ ? inlcrlsystem_->name().str() : survname_.str();
}


void SurveyObject::fillSOPar( IOPar& par, TypeSet<int>& saveids ) const
{
    if ( inlcrlsystem_ )
	par.set( sKeySurvey(), inlcrlsystem_->name() );

    par.setYN( sKeyLocked(), locked_ );
    const int nrattribs = nrAttribs();
    for ( int attrib=nrattribs-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	getSelSpec( attrib )->fillPar( attribpar );

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

	attribpar.set( sKeyTextTrans(), getAttribTransparency( attrib ) );
	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 isAttribEnabled( attrib ) );
									     
	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    const visBase::TextureChannel2RGBA* tc2rgba =
	const_cast<SurveyObject*>(this)->getChannels2RGBA();
    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
		     tc2rgba );

    if ( tc2rgba && !cttc2rgba )
    {
	par.set( sKeyTC2RGBA(), tc2rgba->id() );
	saveids += tc2rgba->id();
    }

    par.set( sKeyNrAttribs(), nrattribs );
}


int SurveyObject::useSOPar( const IOPar& par )
{
    locked_ = false;
    par.getYN( sKeyLocked(), locked_ );

    par.get( sKeySurvey(), survname_ );

    int tc2rgbaid;
    if ( par.get( sKeyTC2RGBA(), tc2rgbaid ) )
    {   
	RefMan<visBase::DataObject> dataobj =
	    visBase::DM().getObject( tc2rgbaid );
	if ( !dataobj )
	    return 0;
				            
	mDynamicCastGet(visBase::TextureChannel2RGBA*, tc2rgba, dataobj.ptr() );
	if ( tc2rgba )
	{
	    if ( !setChannels2RGBA( tc2rgba ) )
		return -1;
	}
    }

    int nrattribs = 0;
    par.get( sKeyNrAttribs(), nrattribs );

    bool firstattrib = true;
    for ( int attrib=0; attrib<nrattribs; attrib++ )
    {
	BufferString key = sKeyAttribs();
	key += attrib;
	PtrMan<const IOPar> attribpar = par.subselect( key );
	if ( !attribpar )
	    continue;

	if ( !firstattrib )
	    addAttrib();
	else
	    firstattrib = false;

	const int attribnr = nrAttribs()-1;

	Attrib::SelSpec spec;
	spec.usePar( *attribpar );
	setSelSpec( attribnr, spec );

	PtrMan<IOPar> seqpar = attribpar->subselect( sKeySequence() );
	ColTab::Sequence seq;
	if ( seqpar )
	{
	    if ( !seq.usePar( *seqpar ) )
	    {
		BufferString seqname;
		if ( seqpar->get( sKey::Name(), seqname ) ) //Sys
		    ColTab::SM().get( seqname.buf(), seq );
	    }
	}
	else //Needed to read horizons written in od4.0
	{
	    seq.usePar( *attribpar );
	}

	setColTabSequence( attribnr, seq, 0 );

	PtrMan<IOPar> mappar = attribpar->subselect( sKeyMapper() );
	ColTab::MapperSetup mapper;
	if ( mappar )
	{
	    mapper.usePar( *mappar );
	    setColTabMapperSetup( attribnr, mapper, 0 );
	}
	else //Needed to read horizons written in od4.0
	{
	    mapper.usePar( *attribpar );
	}

	bool ison = true;
	attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	enableAttrib( attribnr, ison );

	unsigned int trans = 0;
	attribpar->get( sKeyTextTrans(), trans );
	setAttribTransparency( attribnr, trans );
    }

    return 1;
}

}; // namespace visSurvey

