/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: seistrctr.cc,v 1.1.1.2 1999-09-16 09:35:22 arend Exp $";

#include "seistrctr.h"
#include "seistrc.h"
#include "seiskeys.h"
#include "iopar.h"
#include <math.h>
#include <values.h>

UserIDSet SeisTrcTranslator::icparspec("File contains");


const UserIDSet* SeisTrcTranslator::getICParSpec()
{
    if ( !icparspec.size() )
    {
	icparspec.add( "One In-line only" );
	icparspec.add( "Each In-line in one cluster" );
	icparspec.add( "One X-line only" );
	icparspec.add( "Each X-line in one cluster" );
	icparspec.add( "Random sorting" );
    }
    return &icparspec;
}


int SeisTrcTranslator::selector( const char* key )
{
    int retval = defaultSelector( classdef.name(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Integration Framework",key)
      || defaultSelector("Well group",key)
      || defaultSelector("Seismic directory",key) )
	return 1;

    return 0;
}


IOObjContext SeisTrcTranslator::ioContext()
{
    IOObjContext ctxt( Translator::groups()[listid] );
    ctxt.crlink = NO;
    ctxt.newonlevel = 2;
    ctxt.needparent = NO;
    ctxt.selid = "100010";
    ctxt.multi = YES;

    return ctxt;
}


unsigned char* SeisTrcTranslator::trcData( const SeisTrc& trc ) const
{
    return (unsigned char*)trc.data_.data;
}


void SeisTrcTranslator::usePar( const IOPar* iopar )
{
    if ( !iopar ) return;

    keys.usePar( *iopar );
    const char* res = (*iopar)[ icparspec.name() ];
    if ( *res ) slo.fromString( res );
    res = (*iopar)[ "Use coordinates in header" ];
    if ( *res ) trustcoords = yesNoFromString( res );
}
