/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/

static const char* rcsID = "$Id: attribdescsettr.cc,v 1.1 2005-02-03 15:35:02 kristofer Exp $";

#include "attribdescsettr.h"
#include "attribdescset.h"
#include "ioparlist.h"
#include "bufstringset.h"
#include "ioobj.h"
#include "conn.h"
#include "ptrman.h"


const IOObjContext& AttribDescSetTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Attr;
    }

    return *ctxt;
}


int AttribDescSetTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Attribute definitions",key) ) return 1;
    return 0;
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->getTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ads, *conn );
    bool rv = bs == "";
    if ( rv ) bs = tr->warningMsg();
    return rv;
}


bool AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->getTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    bs = tr->write( ads, *conn );
    return bs == "";
}


const char* dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    IOParList iopl( ((StreamConn&)conn).iStream(),
	    	    mTranslGroupName(AttribDescSet) );
    if ( !iopl.size() )
	return "Empty input file";

    IOPar bupar;
    ads.fillPar( bupar );
    ads.removeAll();

    BufferStringSet parseerrmsgs;
    ads.usePar( *iopl[0], &parseerrmsgs );

    if ( !ads.nrDescs() )
    {
	ads.usePar( bupar );
	return "Could not find any attribute definitions in file";
    }
    /*

    int remvd = ads.removeUnused();
    if ( remvd && !ads.nrDescs() )
    {
	parseerrmsgs.erase();
	ads.usePar( *iopl[0], &parseerrmsgs );
    }
    */
    
    if ( parseerrmsgs.size() )
    {
	warningmsg = *parseerrmsgs[0];
	const int nrdispl = parseerrmsgs.size() > 3 ? 4 : parseerrmsgs.size();
	for ( int idx=1; idx<nrdispl; idx++ )
	{
	    warningmsg += "\n";
	    warningmsg += *parseerrmsgs[idx];
	}
	if ( parseerrmsgs.size() > 4 )
	{
	    warningmsg += "\n";
	    warningmsg += "[More warnings omitted]";
	}
    }

    return 0;
}


const char* dgbAttribDescSetTranslator::write( const Attrib::DescSet& ads,
						Conn& conn )
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar* iopar = new IOPar( "Attribute Descriptions" );
    ads.fillPar( *iopar );
    IOParList iopl( mTranslGroupName(AttribDescSet) );
    iopl.deepErase(); // Protection is necessary!
    iopl += iopar;
    if ( !iopl.write(((StreamConn&)conn).oStream()) )
	return "Cannot write attributes to file";

    return 0;
}
