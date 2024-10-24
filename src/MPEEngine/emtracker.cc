/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emtracker.h"

#include "attribsel.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "mpeengine.h"
#include "mpesetup.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"


MPE::EMTracker::EMTracker( EM::EMObject& emobj )
    : emobject_(&emobj)
    , trackingFinished(this)
{
}


MPE::EMTracker::~EMTracker()
{
    deepErase( sectiontrackers_ );
}


ConstRefMan<EM::EMObject> MPE::EMTracker::emObject() const
{
    return emobject_.get();
}


RefMan<EM::EMObject> MPE::EMTracker::emObject()
{
    return emobject_.get();
}


BufferString MPE::EMTracker::objectName() const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    return emobject ? emobject->name() : BufferString::empty();
}


EM::ObjectID MPE::EMTracker::objectID() const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    return emobject ? emobject->id() : EM::ObjectID::udf();
}


bool MPE::EMTracker::snapPositions( const TypeSet<TrcKey>& list )
{
    if ( !emobject_ )
	return false;

    SectionTracker* sectiontracker = getSectionTracker( true );
    if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	return false;

    SectionAdjuster* adjuster = sectiontracker->adjuster();
    if ( !adjuster )
	return false;

    adjuster->reset();
    adjuster->setPositions( list );

    while ( int res=adjuster->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = adjuster->errMsg();
	    return false;
	}
    }

    return true;
}


TrcKeyZSampling MPE::EMTracker::getAttribCube(
					const Attrib::SelSpec& spec ) const
{
    TrcKeyZSampling res( engine().activeVolume() );
    for ( const auto* sectiontracker : sectiontrackers_ )
    {
	TrcKeyZSampling cs = sectiontracker->getAttribCube( spec );
	res.include( cs );
    }

    return res;
}


void MPE::EMTracker::getNeededAttribs( TypeSet<Attrib::SelSpec>& res ) const
{
    for ( const auto* sectiontracker : sectiontrackers_ )
    {
	TypeSet<Attrib::SelSpec> specs;
	sectiontracker->getNeededAttribs( specs );
	for ( const auto& as : specs )
	    res.addIfNew( as );
    }
}

const char* MPE::EMTracker::errMsg() const
{
    return errmsg_.str();
}


MPE::SectionTracker* MPE::EMTracker::cloneSectionTracker()
{
    if ( sectiontrackers_.isEmpty() )
	return nullptr;

    SectionTracker* st = getSectionTracker();
    if ( !st )
	return nullptr;

    SectionTracker* newst = createSectionTracker();
    if ( !newst || !newst->init() )
    {
	delete newst;
	return nullptr;
    }

    IOPar pars;
    st->fillPar( pars );
    newst->usePar( pars );
    newst->reset();
    return newst;
}


MPE::SectionTracker* MPE::EMTracker::getSectionTracker( bool create )
{
    if ( !sectiontrackers_.isEmpty() )
	return sectiontrackers_.first();

    if ( !create )
	return nullptr;

    SectionTracker* sectiontracker = createSectionTracker();
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return nullptr;
    }

    const int defaultsetupidx = sectiontrackers_.size()-1;
    sectiontrackers_ += sectiontracker;

    if ( defaultsetupidx >= 0 )
	applySetupAsDefault();

    return sectiontracker;
}


void MPE::EMTracker::applySetupAsDefault()
{
    SectionTracker* defaultsetuptracker = sectiontrackers_.isEmpty()
					? nullptr : sectiontrackers_.last();
    if ( !defaultsetuptracker || !defaultsetuptracker->hasInitializedSetup() )
	return;

    IOPar par;
    defaultsetuptracker->fillPar( par );
    for ( auto* sectiontracker : sectiontrackers_ )
	if ( !sectiontracker->hasInitializedSetup() )
	    sectiontracker->usePar( par );
}


void MPE::EMTracker::fillPar( IOPar& iopar ) const
{
    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	const SectionTracker* st = sectiontrackers_[idx];
	IOPar localpar;
	localpar.set( sectionidStr(), EM::SectionID::def().asInt() );
	st->fillPar( localpar );

	BufferString key( IOPar::compKey("Section",idx) );
	iopar.mergeComp( localpar, key );
    }
}


bool MPE::EMTracker::usePar( const IOPar& iopar )
{
    int idx=0;
    while ( true )
    {
	BufferString key( IOPar::compKey("Section",idx) );
	PtrMan<IOPar> localpar = iopar.subselect( key );
	if ( !localpar )
	    return true;

	int sid;
	if ( !localpar->get(sectionidStr(),sid) )
	    { idx++; continue; }

	SectionTracker* st = getSectionTracker( true );
	if ( !st )
	{ idx++; continue; }

	MultiID setupid;
	if ( !localpar->get(setupidStr(),setupid) )
	{
	    st->usePar( *localpar );
	    EMSeedPicker* sp = getSeedPicker( false );
	    if ( sp && st->adjuster() )
		sp->setSelSpec( st->adjuster()->getAttributeSel(0) );
	}
	else  // old policy for restoring session
	{
	    st->setSetupID( setupid );
	    PtrMan<IOObj> ioobj = IOM().get( setupid );
	    if ( !ioobj )
		{ idx++; continue; }

	    Setup setup;
	    BufferString bs;
	    if ( !MPESetupTranslator::retrieve(setup,ioobj.ptr(),bs) )
		{ idx++; continue; }

	    IOPar setuppar;
	    setup.fillPar( setuppar );
	    st->usePar( setuppar );
	}

	idx++;
    }

    return true;
}


void MPE::EMTracker::trackingFinishedCB( CallBacker* )
{
    trackingFinished.trigger();
}
