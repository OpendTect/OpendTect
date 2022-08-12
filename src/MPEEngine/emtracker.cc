/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "emtracker.h"

#include "attribsel.h"
#include "autotracker.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "mpeengine.h"
#include "mpesetup.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "undo.h"


namespace MPE
{

mImplFactory1Param( EMTracker, EM::EMObject*, TrackerFactory );

EMTracker::EMTracker( EM::EMObject* emo )
{
    setEMObject( emo );
}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers_ );
    setEMObject( nullptr );
}


BufferString EMTracker::objectName() const
{ return emobject_ ? emobject_->name() : BufferString::empty(); }



EM::ObjectID EMTracker::objectID() const
{
    return emobject_ ? emobject_->id() : EM::ObjectID::udf();
}


bool EMTracker::snapPositions( const TypeSet<TrcKey>& list )
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


TrcKeyZSampling EMTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    TrcKeyZSampling res( engine().activeVolume() );

    for ( int sectidx=0; sectidx<sectiontrackers_.size(); sectidx++ )
    {
	TrcKeyZSampling cs = sectiontrackers_[sectidx]->getAttribCube( spec );
	res.include( cs );
    }


    return res;
}


void EMTracker::getNeededAttribs( TypeSet<Attrib::SelSpec>& res ) const
{
    for ( int sectidx=0; sectidx<sectiontrackers_.size(); sectidx++ )
    {
	TypeSet<Attrib::SelSpec> specs;
	sectiontrackers_[sectidx]->getNeededAttribs( specs );

	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec& as = specs[idx];
	    res.addIfNew( as );
	}
    }
}

const char* EMTracker::errMsg() const
{ return errmsg_.str(); }


SectionTracker* EMTracker::cloneSectionTracker()
{
    if ( sectiontrackers_.isEmpty() )
	return 0;

    SectionTracker* st = getSectionTracker();
    if ( !st ) return 0;

    SectionTracker* newst = createSectionTracker();
    if ( !newst || !newst->init() )
    {
	delete newst;
	return 0;
    }

    IOPar pars;
    st->fillPar( pars );
    newst->usePar( pars );
    newst->reset();
    return newst;
}


SectionTracker* EMTracker::getSectionTracker( bool create )
{
    if ( !sectiontrackers_.isEmpty() )
	return sectiontrackers_.first();

    if ( !create )
	return nullptr;

    SectionTracker* sectiontracker = createSectionTracker();
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return 0;
    }

    const int defaultsetupidx = sectiontrackers_.size()-1;
    sectiontrackers_ += sectiontracker;

    if ( defaultsetupidx >= 0 )
	applySetupAsDefault();

    return sectiontracker;
}


void EMTracker::applySetupAsDefault()
{
    SectionTracker* defaultsetuptracker(0);

    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
	defaultsetuptracker = sectiontrackers_[idx];

    if ( !defaultsetuptracker || !defaultsetuptracker->hasInitializedSetup() )
	return;

    IOPar par;
    defaultsetuptracker->fillPar( par );

    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	if ( !sectiontrackers_[idx]->hasInitializedSetup() )
	    sectiontrackers_[idx]->usePar( par );
    }
}


void EMTracker::fillPar( IOPar& iopar ) const
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


bool EMTracker::usePar( const IOPar& iopar )
{
    int idx=0;
    while ( true )
    {
	BufferString key( IOPar::compKey("Section",idx) );
	PtrMan<IOPar> localpar = iopar.subselect( key );
	if ( !localpar ) return true;

	int sid;
	if ( !localpar->get(sectionidStr(),sid) ) { idx++; continue; }
	SectionTracker* st = getSectionTracker( true );
	if ( !st ) { idx++; continue; }

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
	    if ( !ioobj ) { idx++; continue; }

	    MPE::Setup setup;
	    BufferString bs;
	    if ( !MPESetupTranslator::retrieve(setup,ioobj,bs) )
		{ idx++; continue; }

	    IOPar setuppar;
	    setup.fillPar( setuppar );
	    st->usePar( setuppar );
	}

	idx++;
    }

    return true;
}


void EMTracker::setEMObject( EM::EMObject* no )
{
    if ( emobject_ ) emobject_->unRef();
    emobject_ = no;
    if ( emobject_ ) emobject_->ref();
}

} // namespace MPE
