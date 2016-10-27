/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "randomlineprobe.h"
#include "randomlinegeom.h"
#include "keystrs.h"


mDefineInstanceCreatedNotifierAccess( RDLProbe );

RDLProbe::RDLProbe()
    : Probe()
    , rdlid_(-1)
{
    updateName();
    mTriggerInstanceCreatedNotifier();
}

const char* RDLProbe::sFactoryKey()
{ return IOPar::compKey(sKey::Random(),sKey::Line()); }


const char* RDLProbe::sRandomLineID()
{ return "RandomLineID"; }


BufferString RDLProbe::createName() const
{
    BufferString rdlname;
    const Geometry::RandomLine* rdl = Geometry::RLM().get( rdlid_ );
    if ( rdl )
	rdlname = rdl->name();
    return rdlname;
}


void RDLProbe::fillPar( IOPar& par ) const
{
    Probe::fillPar( par );
    par.set( sRandomLineID(), rdlid_ );
}

bool RDLProbe::usePar( const IOPar& par )
{
    if ( !Probe::usePar(par) )
	return false;

    return par.get( sRandomLineID(), rdlid_ );
}


mImplMonitorableAssignment( RDLProbe, Probe )

void RDLProbe::copyClassData( const RDLProbe& oth )
{
    rdlid_ = oth.randomeLineID();
}


Probe* RDLProbe::createFrom( const IOPar& par )
{
    RDLProbe* probe = new RDLProbe();
    if ( !probe->usePar(par) )
    {
	delete probe;
	return 0;
    }

    return probe;
}


void RDLProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}
