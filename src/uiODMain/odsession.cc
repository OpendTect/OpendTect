/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: odsession.cc,v 1.7 2005-03-09 16:43:22 cvsnanne Exp $";

#include "odsession.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioparlist.h"
#include "ascstream.h"


const char* ODSession::visprefix = "Vis";
const char* ODSession::sceneprefix = "Scene";
const char* ODSession::attrprefix = "Attribs";
const char* ODSession::nlaprefix = "NLA";
const char* ODSession::trackprefix = "Tracking";
const char* ODSession::pluginprefix = "Plugins";


void ODSession::clear()
{
    vispars_.clear();
    scenepars_.clear();
    attrpars_.clear();
    nlapars_.clear();
    mpepars_.clear();
    pluginpars_.clear();
}


ODSession& ODSession::operator=( const ODSession& sess )
{
    if ( &sess != this )
    {
	vispars_ == sess.vispars_;
	scenepars_ == sess.scenepars_;
	attrpars_ == sess.attrpars_;
	nlapars_ == sess.nlapars_;
	mpepars_ == sess.mpepars_;
	pluginpars_ == sess.pluginpars_;
    }
    return *this;
}


bool ODSession::operator==( const ODSession& sess ) const
{
    return vispars_ == sess.vispars_
	&& scenepars_ == sess.scenepars_
	&& attrpars_ == sess.attrpars_
	&& nlapars_ == sess.nlapars_
	&& mpepars_ == sess.mpepars_
	&& pluginpars_ == sess.pluginpars_;
}
    

bool ODSession::usePar( const IOPar& par )
{
    PtrMan<IOPar> vissubpars = par.subselect(visprefix);
    if ( !vissubpars ) return false;
    vispars_ = *vissubpars;

    PtrMan<IOPar> scenesubpars = par.subselect(sceneprefix);
    if ( !scenesubpars || !scenesubpars->size() )
	scenesubpars = par.subselect("Windows"); // backward compat 1.0
    if ( scenesubpars )
        scenepars_ = *scenesubpars;

    PtrMan<IOPar> attrsubpars = par.subselect(attrprefix);
    if ( attrsubpars )
	attrpars_ = *attrsubpars;

    PtrMan<IOPar> nlasubpars = par.subselect(nlaprefix);
    if ( nlasubpars )
	nlapars_ = *nlasubpars;

    PtrMan<IOPar> mpesubpars = par.subselect(trackprefix);
    if ( mpesubpars )
	mpepars_ = *mpesubpars;

    PtrMan<IOPar> pluginsubpars = par.subselect(pluginprefix);
    if ( pluginsubpars )
        pluginpars_ = *pluginsubpars;

    return true;
}


void ODSession::fillPar( IOPar& par ) const
{
    par.mergeComp( vispars_, visprefix );
    par.mergeComp( scenepars_, sceneprefix );
    par.mergeComp( attrpars_, attrprefix );
    par.mergeComp( nlapars_, nlaprefix );
    par.mergeComp( mpepars_, trackprefix );
    par.mergeComp( pluginpars_, pluginprefix );
}


int ODSessionTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector(ODSessionTranslator::keyword,key) ) return 1;
    return 0;
}


const IOObjContext& ODSessionTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    
    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Misc;
    }

    return *ctxt;
}


bool ODSessionTranslator::retrieve( ODSession& session,
				    const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "Cannot find object in data base"; return false; }
    PtrMan<ODSessionTranslator> tr =
		dynamic_cast<ODSessionTranslator*>(ioobj->getTranslator());
    if ( !tr ) { err = "Selected object is not an Session"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ err = "Cannot open "; err += ioobj->fullUserExpr(true); return false;}
    err = tr->read( session, *conn );
    bool rv = err == "";
    if ( rv ) err = tr->warningMsg();
    return rv;
}


bool ODSessionTranslator::store( const ODSession& session,
				 const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "No object to store set in data base"; return false; }
    PtrMan<ODSessionTranslator> tr
	 = dynamic_cast<ODSessionTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ err = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    { err = "Cannot open "; err += ioobj->fullUserExpr(false); return false; }
    err = tr->write( session, *conn );
    return err == "";
}


const char* ODSessionTranslator::keyword = "Session setup";


const char* dgbODSessionTranslator::read( ODSession& session, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );
    bool isoldhdr = !strcmp(astream.projName(),"dGB-dTect");
    int nr = astream.minorVersion() + astream.majorVersion() * 100;
    if ( isoldhdr && nr < 105 )
	return "Cannot read session files older than d-Tect V1.5";

    IOPar iopar( astream );
    if ( !iopar.size() )
	return "Empty input file";

    if ( !session.usePar( iopar ))
	return "Could not read session-file";

    return 0;
}


const char* dgbODSessionTranslator::write( const ODSession& session, Conn& conn)
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar* iopar = new IOPar( ODSessionTranslator::keyword );
    session.fillPar( *iopar );
    IOParList iopl( mTranslGroupName(ODSession) );
    iopl.deepErase(); // Protection is necessary!
    iopl += iopar;
    if ( !iopl.write(((StreamConn&)conn).oStream()) )
	return "Cannot write d-Tect session to file";
    return 0;
}
