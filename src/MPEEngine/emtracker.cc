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

mImplClassFactory( EMTracker, factory );

EMTracker::EMTracker( EM::Object* emo )
    : isenabled_(true)
    , emobject_(0)
    , sectiontracker_(0)
{
    setEMObject(emo);
}


EMTracker::~EMTracker()
{
    delete sectiontracker_;
    setEMObject(0);
}


BufferString EMTracker::objectName() const
{ return emobject_ ? emobject_->name() : BufferString::empty(); }


DBKey EMTracker::objectID() const
{ return emobject_ ? emobject_->id() : DBKey::getInvalid(); }


bool EMTracker::snapPositions( const TypeSet<TrcKey>& list )
{
    if ( !emobject_ ) return false;

    SectionTracker* sectiontracker = getSectionTracker( true );
    if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	return true;

    SectionAdjuster* adjuster = sectiontracker->adjuster();
    if ( !adjuster ) return true;

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

    if ( sectiontracker_ )
	res.include( sectiontracker_->getAttribCube(spec) );

    return res;
}


void EMTracker::getNeededAttribs( Attrib::SelSpecList& res ) const
{
    if ( !sectiontracker_ )
	return;

    Attrib::SelSpecList specs;
    sectiontracker_->getNeededAttribs( specs );

    for ( int idx=0; idx<specs.size(); idx++ )
    {
	const Attrib::SelSpec& as = specs[idx];
	res.addIfNew( as );
    }
}


const char* EMTracker::errMsg() const
{
    return errmsg_;
}


SectionTracker* EMTracker::cloneSectionTracker()
{
    if ( !sectiontracker_ )
	return 0;

    SectionTracker* newst = createSectionTracker();
    if ( !newst || !newst->init() )
    {
	delete newst;
	return 0;
    }

    IOPar pars;
    sectiontracker_->fillPar( pars );
    newst->usePar( pars );
    newst->reset();
    return newst;
}


SectionTracker* EMTracker::getSectionTracker( bool create )
{
    if ( sectiontracker_ )
	return sectiontracker_;

    if ( !create ) return 0;

    SectionTracker* sectiontracker = createSectionTracker();
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return 0;
    }

    sectiontracker_ = sectiontracker;
    return sectiontracker;
}


void EMTracker::fillPar( IOPar& iopar ) const
{
    if ( !sectiontracker_ )
	return;

    IOPar localpar;
    sectiontracker_->fillPar( localpar );

    BufferString key( IOPar::compKey("Section",0) );
    iopar.mergeComp( localpar, key );
}


bool EMTracker::usePar( const IOPar& iopar )
{
    BufferString key( IOPar::compKey("Section",0) );
    PtrMan<IOPar> localpar = iopar.subselect( key );
    if ( !localpar ) return true;

    SectionTracker* st = getSectionTracker( true );
    if ( !st )
       return false;

    DBKey setupid;
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
	PtrMan<IOObj> ioobj = setupid.getIOObj();
	if ( !ioobj )
	    return true;

	MPE::Setup setup;
	uiString bs;
	if ( !MPESetupTranslator::retrieve(setup,ioobj,bs) )
	    return true;

	IOPar setuppar;
	setup.fillPar( setuppar );
	st->usePar( setuppar );
    }

    sectiontracker_ = st;
    return true;
}


void EMTracker::setEMObject( EM::Object* no )
{
    if ( emobject_ ) emobject_->unRef();
    emobject_ = no;
    if ( emobject_ ) emobject_->ref();
}

} // namespace MPE
