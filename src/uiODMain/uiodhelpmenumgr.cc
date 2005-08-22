/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodhelpmenumgr.cc,v 1.1 2005-08-22 07:30:43 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodhelpmenumgr.cc,v 1.1 2005-08-22 07:30:43 cvsbert Exp $";

#include "uiodhelpmenumgr.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uimenu.h"
#include "helpview.h"
#include "dirlist.h"
#include "filepath.h"
#include "filegen.h"

#define mInsertItem(txt,id) \
    helpmnu->insertItem( \
	new uiMenuItem(txt,mCB(mm,uiODMenuMgr,handleClick)), id )


uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
{
    uiPopupMenu* helpmnu = mm->helpMnu();
    DirList dl( GetDataFileName(0), DirList::DirsOnly, "*Doc" );
    bool havedtectdoc = false;
    for ( int hidx=0, idx=0; idx<dl.size(); idx++ )
    {
	BufferString dirnm = dl.get( idx );
	if ( dirnm == "dTectDoc" )
	{
	    havedtectdoc = true;
	    mInsertItem( "&Index ...", mODIndexMnuItm );
	}
	else
	{
	    char* ptr = strstr( dirnm.buf(), "Doc" );
	    if ( !ptr ) continue; // Huh?
	    *ptr = '\0';
	    if ( dirnm == "" ) continue;

	    BufferString itmnm = "&"; // hope there's no duplication
	    itmnm += dirnm; itmnm += "-"; itmnm += "Index ...";
	    mInsertItem( itmnm, mStdHelpMnuBase + hidx + 1 );
	    hidx++;
	}
    }
    if ( havedtectdoc )
    {
	mInsertItem( "Ad&min ...", mAdminMnuItm );
	mInsertItem( "&Programmer ...", mProgrammerMnuItm );
    }
    mInsertItem( "&About ...", mAboutMnuItm );
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


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODHelpMenuMgr::handle( int id, const char* itemname )
{
    switch( id )
    {
    case mAdminMnuItm: 		HelpViewer::doHelp(
				    getHelpF("ApplMan","index.html"),
				    "OpendTect System administrator"); break;
    case mProgrammerMnuItm:	HelpViewer::doHelp(
					getHelpF("Programmer","index.html"),
					"d-Tect" ); break;
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
	if ( id < mStdHelpMnuBase || id > mStdHelpMnuBase + 90 ) return;

	BufferString itmnm( itemname );
	BufferString docnm = "dTect";
	char* ptr = strchr( itmnm.buf(),'-' );
	if ( ptr )
	{
	    *ptr = '\0';
	    docnm = itmnm.buf() + 1; // add one to skip "&"
	}

	BufferString dirnm( docnm ); dirnm += "Doc";
	itmnm = "OpendTect Documentation";
	if ( ptr )
	    { itmnm += " - "; itmnm += docnm; itmnm += " part"; }

	HelpViewer::doHelp( getHelpF(0,"index.html",dirnm.buf()), itmnm );
	break;

    } break;

    }
}
