/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/

static const char* rcsID = "$Id: madprocflow.cc,v 1.7 2010/11/09 16:01:18 cvsbert Exp $";

#include "madprocflow.h"
#include "madprocflowtr.h"
#include "madio.h"
#include "keystrs.h"
#include "seistype.h"
#include "ascstream.h"

const char* ODMad::ProcFlow::sKeyInp()		{ return "Input"; }
const char* ODMad::ProcFlow::sKeyOutp()		{ return "Output"; }
const char* ODMad::ProcFlow::sKeyProc()		{ return "Proc"; }
const char* ODMad::ProcFlow::sKeyNrProcs()	{ return "Nr Procs"; }

defineTranslatorGroup(ODMadProcFlow,"Madagascar process flow");
defineTranslator(dgb,ODMadProcFlow,mDGBKey);
mDefSimpleTranslatorioContextWithExtra(ODMadProcFlow,None,
					ctxt->selkey = ODMad::sKeyMadSelKey())


ODMad::ProcFlow::ProcFlow( const char* nm )
    : NamedObject(nm)
    , inpiop_(sKeyInp())
    , outiop_(sKeyOutp())
{
}


ODMad::ProcFlow::~ProcFlow()
{
    deepErase( *this );
}


ODMad::ProcFlow::IOType ODMad::ProcFlow::ioType( const IOPar& iop )
{
    const char* res = iop.find( sKey::Type );
    if ( !res || !*res || *res == *sKey::None )
	return ODMad::ProcFlow::None;

    if ( *res == *ODMad::sKeyMadagascar() || *res == 'm' )
	return ODMad::ProcFlow::Madagascar;

    if ( *res == 'S' || *res == 's' )
	return ODMad::ProcFlow::SU;

    Seis::GeomType gt = Seis::geomTypeOf( res );
    return (ODMad::ProcFlow::IOType)gt;
}


void ODMad::ProcFlow::setIOType( IOPar& iop, ODMad::ProcFlow::IOType iot )
{
    if ( iot < ODMad::ProcFlow::Madagascar )
	iop.set( sKey::Type, Seis::nameOf((Seis::GeomType)iot) );
    else if ( iot == ODMad::ProcFlow::Madagascar )
	iop.set( sKey::Type, ODMad::sKeyMadagascar() );
    else if ( iot == ODMad::ProcFlow::SU )
	iop.set( sKey::Type, "SU" );
    else
	iop.set( sKey::Type, sKey::None );
}

#define mRetFalse(s) { errmsg = s; return false; }
bool ODMad::ProcFlow::isOK( BufferString& errmsg ) const
{
    if ( !inpiop_.size() )
	mRetFalse("Input parameters missing")
    if ( !outiop_.size() )
	mRetFalse("Output parameters missing")

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !(*this)[idx] )
	    mRetFalse("Empty proc found")
	if ( !(*this)[idx]->isValid() )
	{
	    errmsg = "Invalid command: ";
	    errmsg += (*this)[idx]->getSummary();
	    return false;
	}
    }

    return true;
}
#undef mRetFalse

void ODMad::ProcFlow::fillPar( IOPar& iop ) const
{
    iop.mergeComp( inpiop_, sKeyInp() );
    iop.mergeComp( outiop_, sKeyOutp() );
    iop.set( sKeyNrProcs(), size() );
    for ( int idx=0; idx<size(); idx++ )
    {
	const char* key = IOPar::compKey( sKeyProc(), idx );
	IOPar par;
	(*this)[idx]->fillPar( par );
	iop.mergeComp( par, key );
    }
}


void ODMad::ProcFlow::usePar( const IOPar& iop )
{
    IOPar* subpar = iop.subselect( sKeyInp() );
    if ( subpar && subpar->size() )
	inpiop_ = *subpar;
    else
	inpiop_.setEmpty();
    delete subpar;

    subpar = iop.subselect( sKeyOutp() );
    if ( subpar && subpar->size() )
	outiop_ = *subpar;
    else
	outiop_.setEmpty();
    delete subpar;

    subpar = iop.subselect( sKeyProc() );
    if ( !subpar )
    {
	BufferStringSet procs;
	if ( !iop.get(sKeyProc(),procs) )
	    return;

	for ( int idx=0; idx<procs.size(); idx++ )
	{
	    ODMad::Proc* proc = new ODMad::Proc( procs.get(idx) );
	    (*this) += proc;
	}

	return;
    }

    int nrprocs = mUdf(int);
    iop.get( sKeyNrProcs(), nrprocs );
    for ( int idx=0; idx<nrprocs; idx++ )
    {
	IOPar* procpar = subpar->subselect( idx );
	if ( !procpar ) break;

	ODMad::Proc* proc = new ODMad::Proc("");
	if ( !proc->usePar(*procpar) )
	    return;

	(*this) += proc;
	delete procpar;
    }
}


int ODMadProcFlowTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;
    return defaultSelector("Madagascar data",key) ? 1 : 0;
}


bool ODMadProcFlowTranslator::retrieve( ODMad::ProcFlow& pf, const IOObj* ioobj,
					BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find flow object in data base"; return false; }
    mDynamicCastGet(ODMadProcFlowTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a processing flow"; return false; }
    PtrMan<ODMadProcFlowTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( pf, *conn );
    return bs.isEmpty();
}


bool ODMadProcFlowTranslator::store( const ODMad::ProcFlow& pf,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store flow in data base"; return false; }
    mDynamicCastGet(ODMadProcFlowTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Processing flow"; return false;}

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( pf, *conn );
    delete tr;
    return bs.isEmpty();
}


const char* dgbODMadProcFlowTranslator::read( ODMad::ProcFlow& pf, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(ODMadProcFlow)) )
	return "Input file is not a Processing flow";
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file is empty";

    pf.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );
    IOPar iop( astrm ); pf.usePar( iop );
    return 0;
}


const char* dgbODMadProcFlowTranslator::write( const ODMad::ProcFlow& pf,
						Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(ODMadProcFlow) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Processing flow file";

    IOPar par;
    pf.fillPar( par );
    par.putTo( astrm );
    return strm.good() ? 0 : "Error during write to Processing flow file";
}
