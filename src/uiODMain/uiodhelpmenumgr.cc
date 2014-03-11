/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodhelpmenumgr.h"

#include "uihelpview.h"

#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uidesktopservices.h"
#include "uimenu.h"
#include "filepath.h"
#include "file.h"
#include "oddirs.h"


#define mInsertItem(mnu,txt,id,sc) \
{ \
    uiAction* itm = new uiAction(txt,mCB(mnumgr_,uiODMenuMgr,handleClick));\
    mnu->insertItem( itm, id ); \
    itm->setShortcut( sc ); \
}

static BufferString getAdminURL()
{
    FilePath fp;
    fp = mGetSysAdmDocDir();
    return fp.add("base").add("index.htm").fullPath();
}


uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
	: helpmnu_( mm->helpMnu() )
	, mnumgr_( mm )
{
    if ( HelpProvider::hasHelp(HelpKey(DevDocHelp::sKeyFactoryName(),0)))
	mInsertItem( helpmnu_, "&Programmer ...", mProgrammerMnuItm, 0 );

    BufferString adminurl = getAdminURL();
    if ( File::exists(adminurl) )
	mInsertItem( helpmnu_, "Ad&min ...", mAdminMnuItm, 0 );
    mInsertItem( helpmnu_, "&About", mAboutMnuItm, 0)
    mInsertItem( helpmnu_, "User documentation", mUserDocMnuItm, 0 );
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
}



void uiODHelpMenuMgr::handle( int id )
{
    switch( id )
    {
	case mAdminMnuItm:
	{
	    uiDesktopServices::openUrl( getAdminURL().buf() );
	} break;
	case mProgrammerMnuItm:
	{
	    HelpProvider::provideHelp(HelpKey(DevDocHelp::sKeyFactoryName(),0));
	} break;
	case mAboutMnuItm:
	{
	    uiDesktopServices::openUrl(
		FilePath(GetSoftwareDir(false),"doc","about.html").fullPath() );
	} break;
	default:
	{
	    HelpProvider::provideHelp( HelpKey("od", 0) );
	}
    }
}
