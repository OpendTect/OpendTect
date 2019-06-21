/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/


#include "madprocflow.h"
#include "madprocflowtr.h"
#include "madio.h"
#include "keystrs.h"
#include "seistype.h"
#include "ascstream.h"
#include "uistrings.h"

const char* ODMad::ProcFlow::sKeyInp()		{ return sKey::Input(); }
const char* ODMad::ProcFlow::sKeyOutp()		{ return sKey::Output(); }
const char* ODMad::ProcFlow::sKeyProc()		{ return "Proc"; }
const char* ODMad::ProcFlow::sKeyNrProcs()	{ return "Nr Procs"; }

defineTranslatorGroup(ODMadProcFlow, "Madagascar process flow" );


uiString ODMadProcFlowTranslatorGroup::sTypeName(int)
{ return tr("Madagascar process flow"); }

defineTranslator(dgb,ODMadProcFlow,mDGBKey);
mDefSimpleTranslatorioContextWithExtra( ODMadProcFlow, None,
				ctxt->dirid_.setI( ODMad::cMadDirIDNr() ) )


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
    const char* res = iop.find( sKey::Type() );
    if ( !res || !*res || *res == *sKey::None() )
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
	iop.set( sKey::Type(), Seis::nameOf((Seis::GeomType)iot) );
    else if ( iot == ODMad::ProcFlow::Madagascar )
	iop.set( sKey::Type(), ODMad::sKeyMadagascar() );
    else if ( iot == ODMad::ProcFlow::SU )
	iop.set( sKey::Type(), "SU" );
    else
	iop.set( sKey::Type(), sKey::None() );
}

#define mRetFalse(s) { errmsg = s; return false; }
bool ODMad::ProcFlow::isOK(uiString& errmsg) const
{
    if ( !inpiop_.size() )
	mRetFalse( uiStrings::sParsMissing().addMoreInfo(uiStrings::sInput()) )
    if ( !outiop_.size() )
	mRetFalse( uiStrings::sParsMissing().addMoreInfo(uiStrings::sOutput()) )

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !(*this)[idx] )
	    mRetFalse(tr("Empty proc found"))
	if ( !(*this)[idx]->isValid() )
	{
	    errmsg = tr("Invalid command: %1")
		   .arg((*this)[idx]->getSummary());
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
    int retval = defaultSelector( sGroupName(), key );
    if ( retval ) return retval;
    return defaultSelector("Madagascar data",key) ? 1 : 0;
}


bool ODMadProcFlowTranslator::retrieve( ODMad::ProcFlow& pf, const IOObj* ioobj,
					BufferString& errmsg )
{
    if ( !ioobj )
	{ errmsg = "Cannot find flow object in data base"; return false; }
    mDynamicCast(ODMadProcFlowTranslator*,PtrMan<ODMadProcFlowTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
	{ errmsg = "Selected object is not a processing flow"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        errmsg.set( "Cannot open " ).add( ioobj->fullUserExpr(true) );
    else
	errmsg = mFromUiStringTodo(tr->read( pf, *conn ));
    return errmsg.isEmpty();
}


bool ODMadProcFlowTranslator::store( const ODMad::ProcFlow& pf,
				     const IOObj* ioobj, BufferString& errmsg )
{
    if ( !ioobj )
	{ errmsg = "No object to store flow in data base"; return false; }
    mDynamicCast(ODMadProcFlowTranslator*,PtrMan<ODMadProcFlowTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
	{ errmsg = "Selected object is not a Processing flow"; return false;}

    errmsg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        errmsg.set( "Cannot open " ).add( ioobj->fullUserExpr(false) );
    else
	errmsg = mFromUiStringTodo(tr->write( pf, *conn ));

    if ( !errmsg.isEmpty() )
    {
	if ( conn )
	    conn->rollback();
	return false;
    }

    return true;
}


bool ODMadProcFlowTranslator::retrieve( ODMad::ProcFlow& pf, const IOObj* ioobj,
					uiString& errmsg )
{
    if ( !ioobj ) { errmsg = uiStrings::phrCannotFind(
			    tr("flow object in data base")); return false; }
    mDynamicCast(ODMadProcFlowTranslator*,PtrMan<ODMadProcFlowTranslator> trltr,
		 ioobj->createTranslator());
    if ( !trltr )
    { errmsg = tr("Selected object is not a processing flow"); return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ errmsg = ioobj->phrCannotOpenObj(); return false; }
    errmsg = trltr->read( pf, *conn );
    return errmsg.isEmpty();
}


bool ODMadProcFlowTranslator::store( const ODMad::ProcFlow& pf,
				     const IOObj* ioobj, uiString& errmsg )
{
    if ( !ioobj ) { errmsg = uiStrings::phrCannotSave(tr("flow in data base"));
								return false; }
    mDynamicCast(ODMadProcFlowTranslator*,PtrMan<ODMadProcFlowTranslator> trltr,
		 ioobj->createTranslator());
    if ( !trltr ) { errmsg = tr("Selected object is not a Processing flow");
								return false;}

    errmsg = uiString::empty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ errmsg = ioobj->phrCannotOpenObj(); return false; }
    else
	errmsg = trltr->write( pf, *conn );

    return errmsg.isEmpty();
}


uiString dgbODMadProcFlowTranslator::read( ODMad::ProcFlow& pf, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return mINTERNAL("bad connection");

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotRead(tr("from input file"));
    if ( !astrm.isOfFileType(mTranslGroupName(ODMadProcFlow)) )
	return tr("Input file is not a Processing flow");
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return tr("Input file is empty");

    pf.setName( conn.linkedTo().name() );
    IOPar iop( astrm ); pf.usePar( iop );
    return uiString::empty();
}


uiString dgbODMadProcFlowTranslator::write( const ODMad::ProcFlow& pf,
						Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return mINTERNAL("bad connection");

    const uiString filtypstr = tr("Processing flow file");
    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(ODMadProcFlow) );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotWrite( filtypstr );

    IOPar par;
    pf.fillPar( par );
    par.putTo( astrm );
    return astrm.isOK() ? uiString::empty()
			: uiStrings::phrErrDuringWrite( filtypstr );
}
