/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodhelpmenumgr.cc,v 1.6 2006-11-21 14:00:08 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodhelpmenumgr.cc,v 1.6 2006-11-21 14:00:08 cvsbert Exp $";

#include "uiodhelpmenumgr.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
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
	new uiMenuItem(txt,mCB(mnumgr,uiODMenuMgr,handleClick)), id )

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
    	: havedtectdoc(false)
	, helpmnu(mm->helpMnu())
    	, mnumgr(mm)
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

	di->id = mHelpVarMnuBase + entries.size();
	entries += di;
	if ( dirnm == "dTectDoc" )
	{
	    havedtectdoc = true;
	    if ( entries.size() > 1 )
		entries.swap( 0, entries.size()-1 );
	}
    }

    mkVarMenu();

    if ( havedtectdoc )
    {
	mInsertItem( helpmnu, "Ad&min ...", mAdminMnuItm );
	mInsertItem( helpmnu, "&Programmer ...", mProgrammerMnuItm );
    }
    mInsertItem( helpmnu, "&About ...", mAboutMnuItm );
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
    deepErase( entries );
}


void uiODHelpMenuMgr::insertVarItem( uiPopupMenu* mnu, int eidx, bool withicon )
{
    uiODHelpDocInfo& di = *entries[eidx];
    BufferString txt( di.nm );
    txt += " ...";
    mInsertItem( mnu, txt, di.id );
}


void uiODHelpMenuMgr::mkVarMenu()
{
    if ( entries.size() == 0 )

	ErrMsg( "No help documentation found" );

    else if ( entries.size() == 1 )
    {
	if ( havedtectdoc )
	    entries[0]->nm = "&Index";
	insertVarItem( helpmnu, 0, false );
    }
    else
    {
	uiPopupMenu* submnu = new uiPopupMenu( &mnumgr->appl, "&Index" );
	for ( int idx=0; idx<entries.size(); idx++ )
	    insertVarItem( submnu, idx, true );
	helpmnu->insertItem( submnu );
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
    return fnm.buf();
}


void uiODHelpMenuMgr::handle( int id, const char* itemname )
{
    switch( id )
    {
    case mAdminMnuItm: 
	HelpViewer::doHelp( getHelpF("ApplMan","index.html"),
				     "OpendTect System administrator");
    break;
    case mProgrammerMnuItm:
	HelpViewer::doHelp( getHelpF("Programmer","index.html"), "d-Tect" );
    break;
    case mAboutMnuItm:
    {
	const char* htmlfnm = "about.html";
	const BufferString dddirnm = GetDataFileName( "dTectDoc" );
	BufferString fnm = FilePath(dddirnm).add(htmlfnm).fullPath();
	fnm = File_exists(fnm) ? getHelpF(0,htmlfnm) : htmlfnm;
	HelpViewer::doHelp( fnm, "About OpendTect" );
    } break;

    default:
    {
	uiODHelpDocInfo* di = 0;
	for ( int idx=0; idx<entries.size(); idx++ )
	{
	    if ( entries[idx]->id == id )
		{ di = entries[idx]; break; }
	}
	if ( !di )
	{
	    BufferString msg( "Invalid help menu ID: '" );
	    msg += id; msg += "'"; pErrMsg( msg );
	}
	else
	{
	    BufferString titl( "Documentation for " );
	    titl += di->nm;
	    HelpViewer::use( di->starturl, titl );
	}
    } break;

    }
}
