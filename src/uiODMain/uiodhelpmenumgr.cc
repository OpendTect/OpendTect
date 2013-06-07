/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiodhelpmenumgr.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uidesktopservices.h"
#include "uimenu.h"
#include "ascstream.h"
#include "dirlist.h"
#include "helpview.h"
#include "strmprov.h"
#include "filepath.h"
#include "file.h"
#include "oddirs.h"
#include "iopar.h"
#include "errh.h"

static const char* oddirnm = "base";

#define mInsertItem(mnu,txt,id,sc) \
{ \
    uiMenuItem* itm = new uiMenuItem(txt,mCB(mnumgr_,uiODMenuMgr,handleClick));\
    mnu->insertItem( itm, id ); \
    itm->setShortcut( sc ); \
}

uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
    	: havedtectdoc_(false)
	, helpmnu_(mm->helpMnu())
    	, mnumgr_(mm)
{
    scanEntries( mGetUserDocDir() );
    mkVarMenu();
    if ( havedtectdoc_ )
	mInsertItem( helpmnu_, "Ad&min ...", mAdminMnuItm, 0 );

    const char* programmerdocdir = mGetProgrammerDocDir();
    if ( File::isDirectory(programmerdocdir) )
	mInsertItem( helpmnu_, "&Programmer ...", mProgrammerMnuItm, 0 );

    mkAboutMenu();
    mkCreditsMenu();
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
    deepErase( varentries_ );
    deepErase( aboutentries_ );
    deepErase( creditsentries_ );
}


class uiODHelpDocInfo
{
public:
    			uiODHelpDocInfo() : id(-1)	{}

    int			id;
    BufferString	nm;
    BufferString	iconfnm;
    BufferString	starturl;
    BufferString	shortcut;

    bool		getFrom(std::istream&,const char*);
};


bool uiODHelpDocInfo::getFrom( std::istream& strm, const char* dirnm )
{
    ascistream astrm( strm );

    starturl = "index.htm"; nm = "";
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword("Menu name") )
	    nm = astrm.value();
	else if (  astrm.hasKeyword("Icon file")
		|| astrm.hasKeyword("Start URL") )
	{
	    FilePath fp( astrm.value() );
	    if ( !fp.isAbsolute() )
		fp.setPath( dirnm );
	    (*astrm.keyWord() == 'S' ? starturl : iconfnm) = fp.fullPath();
	}
	else if ( astrm.hasKeyword("Shortcut") )
	    shortcut = astrm.value();
    }

    return !nm.isEmpty();
}


void uiODHelpMenuMgr::scanEntries( const char* docdir )
{
    DirList dl( docdir, DirList::DirsOnly );
    int mnuidx = 1;
    for ( int hidx=0, idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirnm = dl.get( idx );
	if ( dirnm == "Programmer" || dirnm == "Credits" ) continue;

	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	FilePath fp( dl.dirName(), dirnm );
	const BufferString fulldirnm = fp.fullPath();
	fp.add( ".mnuinfo" );
	StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
	if ( !sd.usable() || !di->getFrom(*sd.istrm,fulldirnm) )
	{
	    fp.setFileName( "LinkFileTable.txt" );
	    const bool haslinkfile = File::exists( fp.fullPath() );
	    fp.setFileName( "index.htm" );
	    if ( !File::exists(fp.fullPath()) )
	    {
		fp.setFileName( "index.html" );
		if ( !File::exists(fp.fullPath()) && !haslinkfile )
		    { delete di; sd.close(); continue; }
	    }
	    di->starturl = fp.fullPath();
	    di->iconfnm = FilePath(docdir,"defhelpicon.png").fullPath();
	    di->nm = dirnm;
	}
	sd.close();

	di->id = mHelpVarMnuBase + varentries_.size();
	varentries_ += di;
	if ( dirnm == oddirnm )
	{
	    havedtectdoc_ = true;
	    if ( varentries_.size() > 1 )
		varentries_.swap( 0, varentries_.size()-1 );
	}
    }
}


void uiODHelpMenuMgr::insertVarItem( uiPopupMenu* mnu, int eidx, bool withicon )
{
    uiODHelpDocInfo& di = *varentries_[eidx];
    BufferString txt( di.nm );
    txt += " ...";
    mInsertItem( mnu, txt, di.id, di.shortcut );
}


void uiODHelpMenuMgr::mkVarMenu()
{
    if ( varentries_.size() == 0 )

	ErrMsg( "No help documentation found" );

    else if ( varentries_.size() == 1 )
    {
	if ( havedtectdoc_ )
	    varentries_[0]->nm = "&Index";
	insertVarItem( helpmnu_, 0, false );
    }
    else
    {
	uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&Index" );
	for ( int idx=0; idx<varentries_.size(); idx++ )
	    insertVarItem( submnu, idx, true );
	helpmnu_->insertItem( submnu );
    }
}


void uiODHelpMenuMgr::mkCreditsMenu()
{
    uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&Credits" );
    helpmnu_->insertItem( submnu );

    DirList dl( GetDocFileDir("Credits"), DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	FilePath fp( dl.fullPath(idx), "index.txt" ); IOPar iop;
	if ( !HelpViewer::getCreditsData(fp.fullPath(),iop) )
	    continue;

	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	di->id = mHelpCreditsMnuBase + idx + 1;
	di->nm = iop.name(); di->nm += " ...";
	fp.setFileName( "index.html" );
	di->starturl = fp.fullPath();
	creditsentries_ += di;

	const BufferString dirnm( dl.get(idx) );
	if ( dirnm == "base" && creditsentries_.size() > 1 )
	{
	    di->id = mHelpCreditsMnuBase;
	    creditsentries_.swap( 0, creditsentries_.size()-1 );
	}
    }
    for ( int idx=0; idx<creditsentries_.size(); idx++ )
    {
	const uiODHelpDocInfo* di = creditsentries_[idx];
	mInsertItem( submnu, di->nm, di->id, di->shortcut );
    }
}


void uiODHelpMenuMgr::mkAboutMenu()
{
    uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&About" );
    helpmnu_->insertItem( submnu );
    mInsertItem( submnu, "&General ...", mHelpAboutMnuBase, 0 );

    FilePath fp( GetDocFileDir("ReleaseInfo") );
    DirList dl( fp.fullPath(), DirList::FilesOnly, "*.txt" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString fnm = dl.get( idx );
	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	di->id = mHelpAboutMnuBase + idx + 1;
	di->nm = fnm; di->nm += " ...";
	di->starturl = dl.fullPath(idx);
	aboutentries_ += di;
	if ( fnm == "RELEASE.txt" && aboutentries_.size() > 1 )
	    aboutentries_.swap( 0, aboutentries_.size()-1 );
    }
    for ( int idx=0; idx<aboutentries_.size(); idx++ )
    {
	const uiODHelpDocInfo* di = aboutentries_[idx];
	mInsertItem( submnu, di->nm, di->id, di->shortcut );
    }
}


bool uiODHelpMenuMgr::getPopupData( int id, BufferString& helpurl )
{
    const bool isabout = id < mHelpCreditsMnuBase;
    const bool iscredits = !isabout && id < mHelpVarMnuBase;

    const ObjectSet<uiODHelpDocInfo>& hdi =
	 isabout ? aboutentries_ : (iscredits ? creditsentries_ : varentries_);
    const uiODHelpDocInfo* di = 0;
    for ( int idx=0; idx<hdi.size(); idx++ )
    {
	if ( hdi[idx]->id == id )
	    { di = hdi[idx]; break; }
    }
    if ( !di )
    {
	BufferString msg( "Invalid help menu ID: '" );
	msg += id; msg += "'"; pErrMsg( msg );
	return false;
    }

    helpurl = di->starturl;
    return true;
}


void uiODHelpMenuMgr::handle( int id, const char* itemname )
{
    FilePath fp;
    BufferString helpurl;
    switch( id )
    {
    case mAdminMnuItm:
    {
	fp = mGetSysAdmDocDir();
	helpurl = fp.add("base").add("index.htm").fullPath();
    } break;
    case mProgrammerMnuItm:
    {
	fp = mGetProgrammerDocDir(); 
        helpurl = fp.add("index.html").fullPath();
    } break;
    case mHelpAboutMnuBase:
    {
	fp = mGetUserDocDir();
	const char* htmlfnm = "about.html";
	helpurl = fp.add(oddirnm).add(htmlfnm).fullPath();
	if ( !File::exists(helpurl) )
	    helpurl = GetDocFileDir( htmlfnm );
    } break;
    default:
    {
	if ( !getPopupData( id, helpurl) )
	    return;
    } break;

    }

    if ( !File::exists(helpurl.buf()) )
	helpurl = HelpViewer::getWebUrlFromLocal( helpurl.buf() );
    uiDesktopServices::openUrl( helpurl );
}
