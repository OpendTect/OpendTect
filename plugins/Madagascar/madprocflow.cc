/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/

static const char* rcsID = "$Id: madprocflow.cc,v 1.1 2007-12-19 14:02:44 cvsbert Exp $";

#include "madprocflow.h"
#include "madio.h"
#include "keystrs.h"
#include "seistype.h"

static const char* sKeyInp = "Input";
static const char* sKeyOutp = "Output";
static const char* sKeyProc = "Proc";


ODMad::ProcFlow::ProcFlow()
    : inpiop_(sKeyInp)
    , outiop_(sKeyOutp)
{
}


ODMad::ProcFlow::~ProcFlow()
{
}


ODMad::ProcFlow::IOType ODMad::ProcFlow::ioType( bool inp ) const
{
    const char* res = (inp ? inpiop_ : outiop_).find( sKey::Type );
    if ( !res || !*res || *res == *sKey::None )
	return ODMad::ProcFlow::None;

    if ( *res == *ODMad::sKeyMadagascar || *res == 'm' )
	return ODMad::ProcFlow::Madagascar;

    Seis::GeomType gt = Seis::geomTypeOf( res );
    return (ODMad::ProcFlow::IOType)gt;
}


void ODMad::ProcFlow::setIOType( bool inp, ODMad::ProcFlow::IOType iot )
{
    IOPar& iop = inp ? inpiop_ : outiop_;
    if ( iot < ODMad::ProcFlow::Madagascar )
	iop.set( sKey::Type, Seis::nameOf((Seis::GeomType)iot) );
    else if ( iot == ODMad::ProcFlow::Madagascar )
	iop.set( sKey::Type, ODMad::sKeyMadagascar );
    else
	iop.set( sKey::Type, sKey::None );
}


void ODMad::ProcFlow::fillPar( IOPar& iop ) const
{
    iop.mergeComp( inpiop_, sKeyInp );
    iop.mergeComp( outiop_, sKeyOutp );
    iop.set( sKeyProc, procs_ );
}


void ODMad::ProcFlow::usePar( const IOPar& iop )
{
    IOPar* subpar = iop.subselect( sKeyInp );
    if ( subpar && subpar->size() )
	inpiop_ = *subpar;
    else
	inpiop_.clear();
    delete subpar;

    subpar = iop.subselect( sKeyOutp );
    if ( subpar && subpar->size() )
	outiop_ = *subpar;
    else
	outiop_.clear();
    delete subpar;

    procs_.deepErase();
    iop.get( "Proc", procs_ );
}
