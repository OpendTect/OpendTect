/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "attribdescsettr.h"

#include "ascstream.h"
#include "attrfact.h"
#include "attribdescset.h"
#include "bufstringset.h"
#include "conn.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"


mDefSimpleTranslatorSelector(AttribDescSet, sKeyAttribDescSetTranslatorGroup)
mDefSimpleTranslatorioContext(AttribDescSet, Attr)


static const char* readFromStream( ascistream& astream, Attrib::DescSet& ads,
				   uiString& warningmsg )
{
    if ( mTranslGroupName(AttribDescSet) != astream.fileType() )
	return "File has wrong file type";

    IOPar iopar( astream );
    IOPar bupar; ads.fillPar( bupar );
    ads.removeAll( false );
    TypeSet<uiString> parseerrmsgs;
    ads.usePar( iopar, &parseerrmsgs );
    if ( ads.isEmpty() )
    {
	ads.usePar( bupar );
	return "Could not find any attribute definitions in file";
    }

    if ( parseerrmsgs.size() )
    {
	warningmsg = parseerrmsgs[0];
	const int nrdispl = parseerrmsgs.size() > 3 ? 4 : parseerrmsgs.size();
	for ( int idx = 1; idx<nrdispl; idx++ )
	{
	    warningmsg.append( "\n" );
	    warningmsg.append( parseerrmsgs[idx] );
	}

	if ( parseerrmsgs.size() > 4 )
	{
	    warningmsg.append( "\n" );
	    warningmsg.append( "[More warnings omitted]" );
	}
    }

    return 0;
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const char* fnm, BufferString& bs )
{
    if ( !File::exists(fnm) )
    {
	bs.set("File ").add(fnm).add(" does not exist.");
	return false;
    }

    od_istream odstrm( fnm );
    ascistream astream( odstrm );
    uiString uistr;
    const char* res = readFromStream( astream, ads, uistr );
    bs = uistr.getFullString();
    if ( bs.isEmpty() ) bs.set( res );
    return !res;
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }

    bs = tr->read( ads, *conn );
    bool rv = bs.isEmpty();
    if ( rv ) bs = tr->warningMsg();
    return rv;
}


bool AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    ioobj->pars().set( sKey::Type(), ads.is2D() ? "2D" : "3D" );
    IOM().commitChanges( *ioobj );
    bs = tr->write( ads, *conn );
    return bs.isEmpty();
}


const char* dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warningmsg_.setEmpty();

    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );
    return readFromStream( astream, ads, warningmsg_ );
}


const char* dgbAttribDescSetTranslator::write( const Attrib::DescSet& ads,
						Conn& conn )
{
    warningmsg_.setEmpty();
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iopar( "Attribute Descriptions" );
    ads.fillPar( iopar );
    if ( !iopar.write( ((StreamConn&)conn).oStream(),
		mTranslGroupName(AttribDescSet) ) )
	return "Cannot write attributes to file";

    return 0;
}
