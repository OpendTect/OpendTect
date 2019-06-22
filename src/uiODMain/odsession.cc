/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "odsession.h"

#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "odver.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "od_helpids.h"


const char* ODSession::emprefix()	{ return "EM"; }
const char* ODSession::seisprefix()	{ return "Seis"; }
const char* ODSession::visprefix()	{ return "Vis"; }
const char* ODSession::sceneprefix()	{ return "Scene"; }
const char* ODSession::attr2dprefix()	{ return "2D.Attribs"; }
const char* ODSession::attr3dprefix()	{ return "3D.Attribs"; }
const char* ODSession::nlaprefix()	{ return "NLA"; }
const char* ODSession::trackprefix()	{ return "Tracking"; }
const char* ODSession::pluginprefix()	{ return "Plugins"; }
const char* ODSession::vwr2dprefix()	{ return "2D.Viewer"; }
const char* ODSession::sKeyUseStartup() { return "dTect.Use startup session"; }
const char* ODSession::sKeyStartupID()  { return "Session.Auto ID"; }


ODSession::ODSession()
{
}


void ODSession::clear()
{
    empars_.setEmpty();
    seispars_.setEmpty();
    vispars_.setEmpty();
    scenepars_.setEmpty();
    attrpars2d_.setEmpty();
    attrpars3d_.setEmpty();
    nlapars_.setEmpty();
    mpepars_.setEmpty();
    pluginpars_.setEmpty();
    vwr2dpars_.setEmpty();
}


ODSession& ODSession::operator=( const ODSession& sess )
{
    if ( &sess != this )
    {
	empars_ = sess.empars_;
	seispars_ = sess.seispars_;
	vispars_ = sess.vispars_;
	scenepars_ = sess.scenepars_;
	attrpars2d_ = sess.attrpars2d_;
	attrpars3d_ = sess.attrpars3d_;
	nlapars_ = sess.nlapars_;
	mpepars_ = sess.mpepars_;
	pluginpars_ = sess.pluginpars_;
	vwr2dpars_ = sess.vwr2dpars_;
    }
    return *this;
}


bool ODSession::operator==( const ODSession& sess ) const
{
    return vispars_ == sess.vispars_
	&& empars_ == sess.empars_
	&& seispars_ == sess.seispars_
	&& attrpars2d_ == sess.attrpars2d_
	&& attrpars3d_ == sess.attrpars3d_
	&& nlapars_ == sess.nlapars_
	&& mpepars_ == sess.mpepars_
	&& pluginpars_ == sess.pluginpars_
	&& vwr2dpars_ == sess.vwr2dpars_;
}


bool ODSession::usePar( const IOPar& par )
{
    PtrMan<IOPar> emsubpars = par.subselect( emprefix() );
    if ( emsubpars )
	empars_ = *emsubpars;

    PtrMan<IOPar> seissubpars = par.subselect( seisprefix() );
    if ( seissubpars )
	seispars_ = *seissubpars;

    PtrMan<IOPar> vissubpars = par.subselect( visprefix() );
    if ( !vissubpars ) return false;
    vispars_ = *vissubpars;

    PtrMan<IOPar> scenesubpars = par.subselect( sceneprefix() );
    if ( scenesubpars )
        scenepars_ = *scenesubpars;

    PtrMan<IOPar> attr2dsubpars = par.subselect( attr2dprefix() );
    if ( attr2dsubpars )
	attrpars2d_ = *attr2dsubpars;

    PtrMan<IOPar> attr3dsubpars = par.subselect( attr3dprefix() );
    if ( attr3dsubpars )
	attrpars3d_ = *attr3dsubpars;

    PtrMan<IOPar> nlasubpars = par.subselect( nlaprefix() );
    if ( nlasubpars )
	nlapars_ = *nlasubpars;

    PtrMan<IOPar> mpesubpars = par.subselect( trackprefix() );
    if ( mpesubpars )
	mpepars_ = *mpesubpars;

    PtrMan<IOPar> pluginsubpars = par.subselect( pluginprefix() );
    if ( pluginsubpars )
        pluginpars_ = *pluginsubpars;

    PtrMan<IOPar> vwr2dsubpars = par.subselect( vwr2dprefix() );
    if ( vwr2dsubpars )
        vwr2dpars_ = *vwr2dsubpars;

    return true;
}


void ODSession::fillPar( IOPar& par ) const
{
    par.mergeComp( empars_, emprefix() );
    par.mergeComp( seispars_, seisprefix() );
    par.mergeComp( vispars_, visprefix() );
    par.mergeComp( scenepars_, sceneprefix() );
    par.mergeComp( attrpars2d_, attr2dprefix() );
    par.mergeComp( attrpars3d_, attr3dprefix() );
    par.mergeComp( nlapars_, nlaprefix() );
    par.mergeComp( mpepars_, trackprefix() );
    par.mergeComp( pluginpars_, pluginprefix() );
    par.mergeComp( vwr2dpars_, vwr2dprefix() );
}


IOPar& ODSession::attrpars( bool is2d )
{
    return is2d ? attrpars2d_ : attrpars3d_;
}


void ODSession::getStartupData( bool& douse, DBKey& mid )
{
    Settings::common().getYN( sKeyUseStartup(), douse );
    mid.setInvalid();
    SI().getDefaultPars().get( sKeyStartupID(), mid );
}


void ODSession::setStartupData( bool douse, const DBKey& id )
{
    bool curuse = false; DBKey curid;
    getStartupData( curuse, curid );
    if ( curuse != douse )
    {
	Settings::common().setYN( sKeyUseStartup(), douse );
	Settings::common().write();
    }

    if ( curid != id )
	SI().setDefaultPar( sKeyStartupID(), id.toString(), true );
}

mDefSimpleTranslatorSelector(ODSession);
mDefSimpleTranslatorioContext(ODSession,Misc)


bool ODSessionTranslator::retrieve( ODSession& session,
				    const IOObj* ioobj, uiString& err )
{
    if ( !ioobj )
	{ err = uiStrings::phrCannotFindObjInDB(); return false; }

    PtrMan<ODSessionTranslator> trans =
		dynamic_cast<ODSessionTranslator*>(ioobj->createTranslator());
    if ( !trans )
	{ err = mINTERNAL("Not session transl"); return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ err = ioobj->phrCannotOpenObj(); return false; }

    err = trans->read( session, *conn );
    const bool rv = err.isEmpty();
    if ( rv )
	err = trans->warningMsg();
    return rv;
}


bool ODSessionTranslator::store( const ODSession& session,
				 const IOObj* ioobj, uiString& err )
{
    if ( !ioobj )
	{ err = sNoIoobjMsg(); return false; }

    PtrMan<ODSessionTranslator> trans
	 = dynamic_cast<ODSessionTranslator*>(ioobj->createTranslator());
    if ( !trans )
	{ err = mINTERNAL("Not session transl"); return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ err = ioobj->phrCannotOpenObj(); return false; }

    err = trans->write( session, *conn );
    if ( !err.isEmpty() )
	{ conn->rollback(); return false; }

    return true;
}


uiString dgbODSessionTranslator::read( ODSession& session, Conn& conn )
{
    warningmsg_.setEmpty();
    if ( !conn.forRead() || !conn.isStream() )
	return mINTERNAL("Bad connection [session read]");

    ascistream astream( ((StreamConn&)conn).iStream() );
    IOPar iopar( astream );
    if ( iopar.isEmpty() )
	return tr("No data in session file");

    const auto iopmajver = iopar.odVersion() / 100;
    const auto curmajver = mODVersion / 100;
    if ( iopmajver != curmajver )
	return tr("Cannot read session files from previous major releases");

    if ( !session.usePar(iopar) )
	return uiStrings::phrCannotRead( uiStrings::sSession() );

    return uiString::empty();
}


uiString dgbODSessionTranslator::write( const ODSession& session, Conn& conn)
{
    warningmsg_.setEmpty();
    if ( !conn.forWrite() || !conn.isStream() )
	return mINTERNAL("Bad connection [session write]");

    IOPar iop( ODSessionTranslatorGroup::sGroupName() );
    session.fillPar( iop );
    if ( !iop.write(((StreamConn&)conn).oStream(),mTranslGroupName(ODSession)) )
	return uiStrings::phrCannotWrite( uiStrings::sSession() );

    return uiString::empty();
}



//uiSessionMan
mDefineInstanceCreatedNotifierAccess(uiSessionMan)


uiSessionMan::uiSessionMan( uiParent* p )
: uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(
		    uiStrings::sSession(mPlural)),
		    mNoDlgTitle, mODHelpKey(mSessionManHelpID))
			       .nrstatusflds(1).modal(false),
		   ODSessionTranslatorGroup::ioContext())
{
    createDefaultUI();
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiSessionMan::~uiSessionMan()
{
}


bool uiSessionMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    if ( !curioobj_ )
	{ inf.add( uiStrings::sNoInfoAvailable() ); return false; }

    return true;
}
