/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodhelpmenumgr.cc,v 1.8 2007-05-21 04:40:11 cvsnanne Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodhelpmenumgr.cc,v 1.8 2007-05-21 04:40:11 cvsnanne Exp $";

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


uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
    	: havedtectdoc_(false)
	, helpmnu_(mm->helpMnu())
    	, mnumgr_(mm)
{
    const BufferString datadir( GetDataFileName(0) );
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

	di->id = mHelpVarMnuBase + entries_.size();
	entries_ += di;
	if ( dirnm == "dTectDoc" )
	{
	    havedtectdoc_ = true;
	    if ( entries_.size() > 1 )
		entries_.swap( 0, entries_.size()-1 );
	}
    }

    mkVarMenu();

    if ( havedtectdoc_ )
    {
	mInsertItem( helpmnu_, "Ad&min ...", mAdminMnuItm );
	mInsertItem( helpmnu_, "&Programmer ...", mProgrammerMnuItm );
    }
    mInsertItem( helpmnu_, "&About ...", mAboutMnuItm );
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
    deepErase( entries_ );
}


void uiODHelpMenuMgr::insertVarItem( uiPopupMenu* mnu, int eidx, bool withicon )
{
    uiODHelpDocInfo& di = *entries_[eidx];
    BufferString txt( di.nm );
    txt += " ...";
    mInsertItem( mnu, txt, di.id );
}


void uiODHelpMenuMgr::mkVarMenu()
{
    if ( entries_.size() == 0 )

	ErrMsg( "No help documentation found" );

    else if ( entries_.size() == 1 )
    {
	if ( havedtectdoc_ )
	    entries_[0]->nm = "&Index";
	insertVarItem( helpmnu_, 0, false );
    }
    else
    {
	uiPopupMenu* submnu = new uiPopupMenu( &mnumgr_->appl_, "&Index" );
	for ( int idx=0; idx<entries_.size(); idx++ )
	    insertVarItem( submnu, idx, true );
	helpmnu_->insertItem( submnu );
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
    fnm = GetDataFileName( fnm.buf() );
    return fnm.buf();
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
    case mAboutMnuItm:
    {
	title = "About OpendTect";
	const char* htmlfnm = "about.html";
	const BufferString dddirnm = GetDataFileName( "dTectDoc" );
	helpurl = FilePath(dddirnm).add(htmlfnm).fullPath();
	helpurl = File_exists(helpurl) ? getHelpF(0,htmlfnm) : htmlfnm;
    } break;

    default:
    {
	uiODHelpDocInfo* di = 0;
	for ( int idx=0; idx<entries_.size(); idx++ )
	{
	    if ( entries_[idx]->id == id )
		{ di = entries_[idx]; break; }
	}
	if ( !di )
	{
	    BufferString msg( "Invalid help menu ID: '" );
	    msg += id; msg += "'"; pErrMsg( msg );
	    return;
	}
	else
	{
	    title = "Documentation for "; title += di->nm;
	    helpurl = di->starturl;
	}
    } break;

    }

#ifdef USEQT3
    HelpViewer::doHelp( helpurl, title );
#else
    uiDesktopServices::openUrl( helpurl );
#endif
}
