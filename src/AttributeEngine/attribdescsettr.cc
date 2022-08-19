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
	return mToUiStringTodo("File has wrong file type");

    IOPar iopar( astream );
    IOPar bupar; ads.fillPar( bupar );
    ads.removeAll( false );
    uiStringSet parseerrmsgs;
    ads.usePar( iopar, &parseerrmsgs );
    if ( ads.isEmpty() )
    {
	ads.usePar( bupar );
	return
	    mToUiStringTodo("Could not find any attribute definitions in file");
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
	    warningmsg.append( "[More warnings omitted]", true );
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
					const IOObj* ioobj, uiString& bs)
{
    if (!ioobj) { bs = uiStrings::sCantFindODB(); return false; }
    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if (!trans)
    {
	bs = tr("Selected object is not an Attribute Set");
	return false;
    }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	bs = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(true)));
	return false;
    }

    bs = mToUiStringTodo(trans->read( ads, *conn ));
    bool rv = bs.isEmpty();
    if ( rv ) bs = mToUiStringTodo(trans->warningMsg());
    return rv;
}


bool AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, uiString& bs )
{
    if (!ioobj)
    {
	bs = sNoIoobjMsg(); return false;
    }
    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if (!trans)
    {
	bs = tr("Selected object is not an Attribute Set"); return false;
    }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	bs = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(false)));
	return false;
    }
    ioobj->pars().set( sKey::Type(), ads.is2D() ? "2D" : "3D" );
    IOM().commitChanges( *ioobj );
    bs = mToUiStringTodo(trans->write( ads, *conn ));
    return bs.isEmpty();
}


const char* dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warningmsg_.setEmpty();

    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );
    return mFromUiStringTodo(readFromStream( astream, ads, warningmsg_ ));
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
