/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odsession.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "odver.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"

#include "uitextedit.h"


const char* ODSession::visprefix()	{ return "Vis"; }
const char* ODSession::sceneprefix()	{ return "Scene"; }
const char* ODSession::attrprefix()	{ return "Attribs"; }//backward comp 2.4
const char* ODSession::attr2dprefix()	{ return "2D.Attribs"; }
const char* ODSession::attr3dprefix()	{ return "3D.Attribs"; }
const char* ODSession::attr2dstoredprefix()  { return "2D.Stored.Attribs"; }
const char* ODSession::attr3dstoredprefix()  { return "3D.Stored.Attribs"; }
const char* ODSession::nlaprefix()	{ return "NLA"; }
const char* ODSession::trackprefix()	{ return "Tracking"; }
const char* ODSession::pluginprefix()	{ return "Plugins"; }
const char* ODSession::vwr2dprefix()	{ return "2D.Viewer"; }
const char* ODSession::sKeyUseStartup() { return "dTect.Use startup session"; }
const char* ODSession::sKeyStartupID()  { return "Session.Auto ID"; }


ODSession::ODSession()
{
    versionnr_ = mODMajorVersion; versionnr_ += ".";
    versionnr_ += mODMinorVersion;
}


void ODSession::clear()
{
    vispars_.setEmpty();
    scenepars_.setEmpty();
    attrpars_.setEmpty();				//backward comp 2.4
    attrpars2d_.setEmpty();
    attrpars3d_.setEmpty();
    attrpars2dstored_.setEmpty();
    attrpars3dstored_.setEmpty();
    nlapars_.setEmpty();
    mpepars_.setEmpty();
    pluginpars_.setEmpty();
    vwr2dpars_.setEmpty();
}


ODSession& ODSession::operator=( const ODSession& sess )
{
    if ( &sess != this )
    {
	vispars_ = sess.vispars_;
	scenepars_ = sess.scenepars_;
	attrpars_ = sess.attrpars_;		//backward comp 2.4
	attrpars2d_ = sess.attrpars2d_;
	attrpars3d_ = sess.attrpars3d_;
	attrpars2dstored_ = sess.attrpars2dstored_;
	attrpars3dstored_ = sess.attrpars3dstored_;
	nlapars_ = sess.nlapars_;
	mpepars_ = sess.mpepars_;
	pluginpars_ = sess.pluginpars_;
	versionnr_ = sess.versionnr_;
	vwr2dpars_ = sess.vwr2dpars_;
    }
    return *this;
}


bool ODSession::operator==( const ODSession& sess ) const
{
    return vispars_ == sess.vispars_
	&& attrpars_ == sess.attrpars_		//backward comp 2.4
	&& attrpars2d_ == sess.attrpars2d_
	&& attrpars3d_ == sess.attrpars3d_
	&& attrpars2dstored_ == sess.attrpars2dstored_
	&& attrpars3dstored_ == sess.attrpars3dstored_
	&& nlapars_ == sess.nlapars_
	&& mpepars_ == sess.mpepars_
	&& pluginpars_ == sess.pluginpars_
	&& versionnr_ == sess.versionnr_
	&& vwr2dpars_ == sess.vwr2dpars_;
}
   
#define mAddVersionNr(iopar) \
    iopar.add( sKey::Version(), versionnr_ );

bool ODSession::usePar( const IOPar& par )
{
    PtrMan<IOPar> vissubpars = par.subselect(visprefix());
    if ( !vissubpars ) return false;
    vispars_ = *vissubpars;

    PtrMan<IOPar> scenesubpars = par.subselect(sceneprefix());
    if ( scenesubpars )
        scenepars_ = *scenesubpars;

    PtrMan<IOPar> attrsubpars = par.subselect(attrprefix());
    if ( attrsubpars )
	attrpars_ = *attrsubpars;		//backward comp 2.4

    PtrMan<IOPar> attr2dsubpars = par.subselect(attr2dprefix());
    if ( attr2dsubpars )
	attrpars2d_ = *attr2dsubpars;

    PtrMan<IOPar> attr3dsubpars = par.subselect(attr3dprefix());
    if ( attr3dsubpars )
	attrpars3d_ = *attr3dsubpars;
    
    PtrMan<IOPar> attr2dstoredsubpars = par.subselect(attr2dstoredprefix());
    if ( attr2dstoredsubpars )
	attrpars2dstored_ = *attr2dstoredsubpars;

    PtrMan<IOPar> attr3dstoredsubpars = par.subselect(attr3dstoredprefix());
    if ( attr3dstoredsubpars )
	attrpars3dstored_ = *attr3dstoredsubpars;
    
    PtrMan<IOPar> nlasubpars = par.subselect(nlaprefix());
    if ( nlasubpars )
	nlapars_ = *nlasubpars;

    PtrMan<IOPar> mpesubpars = par.subselect(trackprefix());
    if ( mpesubpars )
	mpepars_ = *mpesubpars;

    PtrMan<IOPar> pluginsubpars = par.subselect(pluginprefix());
    if ( pluginsubpars )
        pluginpars_ = *pluginsubpars;

    PtrMan<IOPar> vwr2dsubpars = par.subselect(vwr2dprefix());
    if ( vwr2dsubpars )
        vwr2dpars_ = *vwr2dsubpars;

    mAddVersionNr(vispars_);
    mAddVersionNr(scenepars_);
    mAddVersionNr(attrpars_);
    mAddVersionNr(attrpars2d_);
    mAddVersionNr(attrpars3d_);
    mAddVersionNr(attrpars2dstored_);
    mAddVersionNr(attrpars3dstored_);
    mAddVersionNr(nlapars_);
    mAddVersionNr(mpepars_);
    mAddVersionNr(pluginpars_);
    mAddVersionNr(vwr2dpars_);
    return true;
}


void ODSession::fillPar( IOPar& par ) const
{
    par.mergeComp( vispars_, visprefix() );
    par.mergeComp( scenepars_, sceneprefix() );
    par.mergeComp( attrpars_, attrprefix() );	//backward comp 2.4
    par.mergeComp( attrpars2d_, attr2dprefix() );
    par.mergeComp( attrpars3d_, attr3dprefix() );
    par.mergeComp( attrpars2dstored_, attr2dstoredprefix() );
    par.mergeComp( attrpars3dstored_, attr3dstoredprefix() );
    par.mergeComp( nlapars_, nlaprefix() );
    par.mergeComp( mpepars_, trackprefix() );
    par.mergeComp( pluginpars_, pluginprefix() );
    par.mergeComp( vwr2dpars_, vwr2dprefix() );
}


IOPar& ODSession::attrpars( bool is2d, bool isstored )
{
    if ( !attrpars2d_.size() && !attrpars3d_.size() && attrpars_.size() )
	return attrpars_;			//backward comp 2.4
    else if ( is2d )
	return isstored ? attrpars2dstored_ : attrpars2d_;
    else
	return isstored ? attrpars3dstored_ : attrpars3d_;
}


void ODSession::getStartupData( bool& douse, MultiID& id )
{
    Settings::common().getYN( sKeyUseStartup(), douse );
    id = SI().pars().find( sKeyStartupID() );
}


void ODSession::setStartupData( bool douse, const MultiID& id )
{
    bool curuse = false; MultiID curid;
    getStartupData( curuse, curid );
    if ( curuse != douse )
    {
	Settings::common().setYN( sKeyUseStartup(), douse );
	Settings::common().write();
    }

    if ( curid != id )
    {
	SI().getPars().set( sKeyStartupID(), (const char*)id );
	SI().savePars();
    }
}


int ODSessionTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector(ODSessionTranslator::keyword(),key) ) return 1;
    return 0;
}

mDefSimpleTranslatorioContext(ODSession,Misc)


bool ODSessionTranslator::retrieve( ODSession& session,
				    const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "Cannot find object in data base"; return false; }
    PtrMan<ODSessionTranslator> tr =
		dynamic_cast<ODSessionTranslator*>(ioobj->createTranslator());
    if ( !tr ) { err = "Selected object is not an Session"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ err = "Cannot open "; err += ioobj->fullUserExpr(true); return false;}
    err = tr->read( session, *conn );
    bool rv = err.isEmpty();
    if ( rv ) err = tr->warningMsg();
    return rv;
}


bool ODSessionTranslator::store( const ODSession& session,
				 const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "No object to store set in data base"; return false; }
    PtrMan<ODSessionTranslator> tr
	 = dynamic_cast<ODSessionTranslator*>(ioobj->createTranslator());
    if ( !tr )
	{ err = "Selected object is not an OpendTect Session"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    { err = "Cannot open "; err += ioobj->fullUserExpr(false); return false; }
    err = tr->write( session, *conn );
    return err.isEmpty();
}


const char* ODSessionTranslator::keyword() { return "Session setup"; }


const char* dgbODSessionTranslator::read( ODSession& session, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( astream.majorVersion() < 4 )
	return "Cannot read session files older than OpendTect V4.0";

    IOPar iopar( astream );
    if ( iopar.isEmpty() )
	return "Empty input file";

    session.setVersionNr( BufferString(astream.version()) );
    if ( !session.usePar( iopar ))
	return "Could not read session-file";

    return 0;
}


const char* dgbODSessionTranslator::write( const ODSession& session, Conn& conn)
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iop( ODSessionTranslator::keyword() );
    session.fillPar( iop );
    if ( !iop.write(((StreamConn&)conn).oStream(),mTranslGroupName(ODSession)) )
	return "Cannot write d-Tect session to file";
    return 0;
}



mDefineInstanceCreatedNotifierAccess(uiSessionMan)


uiSessionMan::uiSessionMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Sessions",
				     mNoDlgTitle,"101.3.0").nrstatusflds(1),
		   ODSessionTranslatorGroup::ioContext())
{
    createDefaultUI();
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiSessionMan::~uiSessionMan()
{
}


void uiSessionMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    txt += getFileInfo();
    setInfo( txt );
}
