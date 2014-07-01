/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodhelpmenumgr.h"

#include "uidesktopservices.h"
#include "uihelpview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"

#include "buildinfo.h"
#include "filepath.h"
#include "file.h"
#include "oddirs.h"
#include "od_istream.h"
#include "odver.h"
#include "osgver.h"


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
    mInsertItem( helpmnu_, tr("User documentation"), mUserDocMnuItm, "F1" );

    if ( HelpProvider::hasHelp(HelpKey(DevDocHelp::sKeyFactoryName(),0)))
	mInsertItem( helpmnu_, tr("&Programmer ..."), mProgrammerMnuItm, 0 );

    BufferString adminurl = getAdminURL();
    if ( File::exists(adminurl) )
	mInsertItem( helpmnu_, tr("Ad&min ..."), mAdminMnuItm, 0 );

    mInsertItem( helpmnu_, tr("&About"), mAboutMnuItm, 0)
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
	    uiMSG().aboutOpendTect( getAboutString() );
	} break;
	default:
	{
	    HelpProvider::provideHelp( HelpKey("od", 0) );
	}
    }
}


BufferString uiODHelpMenuMgr::getAboutString()
{
    BufferString str( "<html>" );
    str.set( "<h2>OpendTect v" ).add( GetFullODVersion() ).add("</h2><br>");

    str.add( "Built on " ).add( mBUILD_DATE ).add( "<br>" )
       .add( "From SVN repository: " ).add( mSVN_URL ).add( "<br>" )
       .add( "revision " ).add( mSVN_VERSION ).add( "<br><br>" );

    str.add( "Based on Qt " ).add( GetQtVersion() )
       .add( ", OSG " ).add( GetOSGVersion() )
       .add( ", GCC " ).add( GetGCCVersion() ).add( "<br><br>" );

    str.add( mCOPYRIGHT_STRING ).add( "<br>" )
       .add( "OpendTect is released under a triple licensing scheme. "
	     "For more information, click "
	     "<a href=\"http://dgbes.com/index.php/products/licenses\">"
	     "here</a>.<br>" );
    str.add( "</html>" );
    return str;
}
