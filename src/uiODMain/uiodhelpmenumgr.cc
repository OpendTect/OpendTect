/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodhelpmenumgr.cc,v 1.11 2007-09-12 14:42:27 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodhelpmenumgr.cc,v 1.11 2007-09-12 14:42:27 cvsbert Exp $";

#include "uiodhelpmenumgr.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uidesktopservices.h"
#include "uimenu.h"
#include "dirlist.h"
#include "helpview.h"
#include "ascstream.h"
#include "strmprov.h"
#include "filepath.h"
#include "filegen.h"
#include "oddirs.h"
#include "errh.h"

#define mInsertItem(mnu,txt,id) \
    mnu->insertItem( \
	new uiMenuItem(txt,mCB(mnumgr_,uiODMenuMgr,handleClick)), id )

uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
    	: havedtectdoc_(false)
	, helpmnu_(mm->helpMnu())
    	, mnumgr_(mm)
{
    const BufferString datadir( mGetSWDirDataDir() );
    scanEntries( datadir );
    mkVarMenu();
    if ( havedtectdoc_ )
    {
	mInsertItem( helpmnu_, "Ad&min ...", mAdminMnuItm );
	mInsertItem( helpmnu_, "&Programmer ...", mProgrammerMnuItm );
    }
    mkAboutMenu( datadir );
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

    bool		getFrom(std::istream&,const char*);
};


bool uiODHelpDocInfo::getFrom( std::istream& strm, const char* dirnm )
{
    ascistream astrm( strm );

    starturl = "index.html"; nm = "";
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
    }

    return !nm.isEmpty();
}


void uiODHelpMenuMgr::scanEntries( const char* datadir )
{
    DirList dl( datadir, DirList::DirsOnly, "*Doc" );
    int mnuidx = 1;
    for ( int hidx=0, idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirnm = dl.get( idx );
	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	FilePath fp( dl.dirName() ); fp.add( dirnm );
	const BufferString fulldirnm = fp.fullPath();
	fp.add( ".mnuinfo" );
	StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
	if ( !sd.usable() || !di->getFrom(*sd.istrm,fulldirnm) )
	{
	    fp.setFileName( "index.html" );
	    if ( !File_exists(fp.fullPath()) )
		{ delete di; sd.close(); continue; }
	    di->starturl = fp.fullPath();
	    FilePath iconfp( datadir );
	    di->iconfnm = iconfp.add( "defhelpicon.png" ).fullPath();
	    di->nm = dirnm;
	    *(strstr(di->nm.buf(),"Doc")) = '\0';
	}
	sd.close();

	di->id = mHelpVarMnuBase + varentries_.size();
	varentries_ += di;
	if ( dirnm == "dTectDoc" )
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
    mInsertItem( mnu, txt, di.id );
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


void uiODHelpMenuMgr::mkAboutMenu( const char* datadir )
{
    uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&About" );
    helpmnu_->insertItem( submnu );
    mInsertItem( submnu, "&General ...", mHelpAboutMnuBase );

    FilePath fp( datadir ); fp.add( "ReleaseInfo" );
    DirList dl( fp.fullPath() );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	uiODHelpDocInfo* di = new uiODHelpDocInfo;
	const BufferString fnm = dl.get( idx );
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
	mInsertItem( submnu, di->nm, di->id );
    }
}


static const char* getHelpF( const char* subdir, const char* infnm,
			     const char* basedirnm = "dTectDoc" )
{
    FilePath fp( basedirnm );
    if ( subdir && *subdir ) fp.add( subdir );
    fp.add( infnm );
    static BufferString fnm;
    fnm = fp.fullPath();
    fnm = GetSetupDataFileName( ODSetupLoc_SWDirOnly, fnm.buf() );
    return fnm.buf();
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

    title = isabout ? "" : "Documentation for ";
    title += di->nm;
    helpurl = di->starturl;
    return true;
}


void uiODHelpMenuMgr::handle( int id, const char* itemname )
{
    BufferString helpurl;
    BufferString title;
    switch( id )
    {
    case mAdminMnuItm:
	title = "OpendTect System administrator";
	helpurl = getHelpF( "ApplMan", "index.html" );
    break;
    case mProgrammerMnuItm:
	title = "d-Tect";
        helpurl = getHelpF( "Programmer", "index.html" );
    break;
    case mHelpAboutMnuBase:
    {
	title = "About OpendTect";
	const char* htmlfnm = "about.html";
	const BufferString dddirnm
		= GetSetupDataFileName( ODSetupLoc_SWDirOnly, "dTectDoc" );
	helpurl = FilePath(dddirnm).add(htmlfnm).fullPath();
	helpurl = File_exists(helpurl) ? getHelpF(0,htmlfnm) : htmlfnm;
    } break;
    default:
    {
	if ( !getPopupData( id, title, helpurl) )
	    return;
    } break;

    }

#ifdef USEQT3
    HelpViewer::doHelp( helpurl, title );
#else
    uiDesktopServices::openUrl( helpurl );
#endif
}
