/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodhelpmenumgr.cc,v 1.15 2008-06-05 09:17:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiodhelpmenumgr.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uidesktopservices.h"
#include "uimenu.h"
#include "ascstream.h"
#include "dirlist.h"
#include "strmprov.h"
#include "filepath.h"
#include "filegen.h"
#include "oddirs.h"
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
    {
	mInsertItem( helpmnu_, "Ad&min ...", mAdminMnuItm, 0 );
	mInsertItem( helpmnu_, "&Programmer ...", mProgrammerMnuItm, 0 );
    }
    mkAboutMenu();
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
    deepErase( varentries_ );
    deepErase( aboutentries_ );
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
	if ( dirnm == "Programmer" ) continue;

	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	FilePath fp( dl.dirName() ); fp.add( dirnm );
	const BufferString fulldirnm = fp.fullPath();
	fp.add( ".mnuinfo" );
	StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
	if ( !sd.usable() || !di->getFrom(*sd.istrm,fulldirnm) )
	{
	    fp.setFileName( "index.htm" );
	    if ( !File_exists(fp.fullPath()) )
	    {
		fp.setFileName( "index.html" );
		if ( !File_exists(fp.fullPath()) )
		    { delete di; sd.close(); continue; }
	    }
	    di->starturl = fp.fullPath();
	    FilePath iconfp( docdir );
	    di->iconfnm = iconfp.add( "defhelpicon.png" ).fullPath();
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


void uiODHelpMenuMgr::mkAboutMenu()
{
    uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&About" );
    helpmnu_->insertItem( submnu );
    mInsertItem( submnu, "&General ...", mHelpAboutMnuBase, 0 );

    FilePath fp( GetDocFileDir("ReleaseInfo") );
    DirList dl( fp.fullPath(), DirList::FilesOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	if ( File_isDirectory(dl.fullPath(idx)) )
	    continue;

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


bool uiODHelpMenuMgr::getPopupData( int id, BufferString& title,
				    BufferString& helpurl )
{
    const bool isabout = id < mHelpVarMnuBase;

    const ObjectSet<uiODHelpDocInfo>& hdi =
				 isabout ? aboutentries_ : varentries_;
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
	if ( !File_exists(helpurl) )
	    helpurl = GetDocFileDir( htmlfnm );
    } break;
    default:
    {
	BufferString title;
	if ( !getPopupData( id, title, helpurl) )
	    return;
    } break;

    }

    uiDesktopServices::openUrl( helpurl );
}
