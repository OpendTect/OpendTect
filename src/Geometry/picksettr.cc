/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jul 2005
 RCS:		$Id: picksettr.cc,v 1.6 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "picksetfact.h"
#include "pickset.h"
#include "ctxtioobj.h"
#include "binidvalset.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "streamconn.h"
#include "ioman.h"
#include "errh.h"
#include "keystrs.h"


const IOObjContext& PickSetTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = true;
	ctxt->stdseltype = IOObjContext::Loc;
    }

    return *ctxt;
}


int PickSetTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Miscellaneous directory",key)
      || defaultSelector("Locations directory",key) ) return 1;

    return 0;
}


bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Pick Set"; return false; }
    PtrMan<PickSetTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ps, *conn );
    return bs == "";
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
				BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Pick Set"; return false; }

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( ps, *conn );
    delete tr;
    return bs == "";
}


const char* dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return "Input file is not a Pick Set";
    if ( atEndOfSection(astrm) ) astrm.next();

    float zfac = 1;
    if ( astrm.hasKeyword("Z Factor") )
    {
	zfac = astrm.getValue();
	if ( mIsZero(zfac,mDefEps) || mIsUdf(zfac) )
	    zfac = 1;
	astrm.next();
	if ( atEndOfSection(astrm) ) astrm.next();
    }

    if ( atEndOfSection(astrm) )
	return "Input file contains no pick sets";

    ps.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    Pick::Location loc;
    // In old format we can find mulitple pick sets. Just gather them all
    // in the pick set
    for ( int ips=0; !atEndOfSection(astrm); ips++ )
    {
	astrm.next();
	if ( astrm.hasKeyword(sKey::Color) )
	{
	    ps.color_.use( astrm.value() );
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKey::Size) )
	{
	    ps.pixsize_ = astrm.getVal();
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    loc.pos.z *= zfac;
	    ps += loc;
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();
    }

    return ps.size() ? 0 : "No valid picks found";
}


const char* dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSet) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    astrm.put( "Ref", ps.name() );
    char buf[80];
    if ( ps.color_ != Color::NoColor )
    {
	ps.color_.fill( buf );
	astrm.put( sKey::Color, buf );
    }
    if ( ps.pixsize_ != 0 )
	astrm.put( sKey::Size, ps.pixsize_ );

    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	ps[iloc].toString( buf );
	strm << buf << '\n';
    }

    astrm.newParagraph();
    return strm.good() ? 0
	:  "Error during write to output Pick Set file";
}


void PickSetTranslator::createBinIDValueSets(
			const BufferStringSet& ioobjids,
			ObjectSet<BinIDValueSet>& bivsets )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	MultiID key( ioobjids.get( idx ) );
	PtrMan<IOObj>ioobj = IOM().get( key );
	BufferString msg;
	if ( !ioobj )
	{
	    msg = "Cannot find PickSet with key "; msg += key;
	    ErrMsg( msg ); continue;
	}
	Pick::Set ps;
	if ( !retrieve(ps,ioobj,msg) )
	    { ErrMsg( msg ); continue; }

	const int nrpicks = ps.size();
	if ( !nrpicks ) continue;

	BinIDValueSet* bs = new BinIDValueSet( 1, true );

	for ( int ipck=0; ipck<nrpicks; ipck++ )
	{
	    Pick::Location pl( ps[ipck] );
	    bs->add( SI().transform(pl.pos), pl.pos.z );
	}

	bivsets += bs;
    }
}
