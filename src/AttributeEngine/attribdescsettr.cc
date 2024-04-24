/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "uistrings.h"

static uiString readFromStream( ascistream& astream, Attrib::DescSet& ads,
				uiString& warningmsg )
{
    if ( mTranslGroupName(AttribDescSet) != astream.fileType() )
	return od_static_tr("readFromStream","File has wrong file type");

    IOPar iopar( astream );
    IOPar bupar; ads.fillPar( bupar );
    ads.removeAll( false );
    uiStringSet parseerrmsgs;
    ads.usePar( iopar, &parseerrmsgs );
    if ( ads.isEmpty() )
    {
	ads.usePar( bupar );
	return od_static_tr("readFromStream",
			"Could not find any attribute definitions in file");
    }

    if ( parseerrmsgs.size() )
    {
	warningmsg = parseerrmsgs[0];
	const int nrdispl = parseerrmsgs.size() > 3 ? 4 : parseerrmsgs.size();
	for ( int idx = 1; idx<nrdispl; idx++ )
	{
	    warningmsg.append( parseerrmsgs[idx], true );
	}

	if ( parseerrmsgs.size() > 4 )
	{
	    const uiString msg = od_static_tr("readFromStream",
						"[More warnings omitted]");
	    warningmsg.append( msg, true );
	}
    }

    return uiString::emptyString();
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const char* fnm, uiString& bs )
{
    if ( !File::exists(fnm) )
    {
	tr("File %1 does not exist.").arg(fnm);
	return false;
    }

    od_istream odstrm( fnm );
    ascistream astream( odstrm );
    uiString uistr;
    const uiString res = readFromStream( astream, ads, uistr );
    bs = uistr;
    if (bs.isEmpty())
	bs = res;

    return res.isEmpty();
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const IOObj* ioobj, uiString& errmsg )
{
    if (!ioobj)
    {
	errmsg = uiStrings::sCantFindODB();
	return false;
    }

    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if ( !trans )
    {
	errmsg = tr("Selected object is not an Attribute Set");
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen(
				    toUiString(ioobj->fullUserExpr(true)) );
	return false;
    }

    errmsg = trans->read( ads, *conn );
    const bool rv = errmsg.isEmpty();
    if ( rv )
	errmsg = trans->warningMsg();

    return rv;
}


bool AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = sNoIoobjMsg();
	return false;
    }

    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if (!trans)
    {
	errmsg = tr("Selected object is not an Attribute Set");
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen(
				    toUiString(ioobj->fullUserExpr(false)) );
	return false;
    }

    ioobj->pars().set( sKey::Type(), ads.is2D() ? "2D" : "3D" );
    if ( !IOM().commitChanges(*ioobj) );
    {
	errmsg = uiStrings::phrCannotWriteDBEntry(toUiString(ioobj->name()));
	return false;
    }

    errmsg = trans->write( ads, *conn );
    return errmsg.isEmpty();
}


uiString dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warningmsg_.setEmpty();

    if ( !conn.forRead() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    ascistream astream( ((StreamConn&)conn).iStream() );
    return readFromStream( astream, ads, warningmsg_ );
}


uiString dgbAttribDescSetTranslator::write( const Attrib::DescSet& ads,
						Conn& conn )
{
    warningmsg_.setEmpty();
    if ( !conn.forWrite() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    IOPar iopar( "Attribute Descriptions" );
    ads.fillPar( iopar );
    if ( !iopar.write( ((StreamConn&)conn).oStream(),
		mTranslGroupName(AttribDescSet) ) )
	return tr("Cannot write attributes to file");

    return uiString::empty();
}
