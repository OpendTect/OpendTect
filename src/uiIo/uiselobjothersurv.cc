/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiselobjothersurv.cc,v 1.1 2010-12-14 08:50:32 cvsbruno Exp $";

#include "uiselobjothersurv.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "uilistbox.h"
#include "uiioobjsel.h"
#include "uimsg.h"


static BufferStringSet getSurvList() 
{
    BufferStringSet bss; bss.erase();
    const char* basedir = GetBaseDataDir();
    if ( basedir ) 
    {
	PtrMan<DirList> dirlist = new DirList( basedir, DirList::DirsOnly );
	for ( int idx=0; idx<dirlist->size(); idx++ )
	{
	    const char* dirnm = dirlist->get( idx );
	    if ( matchString("_New_Survey_",dirnm) )
		continue;

	    FilePath fp( basedir );
	    fp.add( dirnm ).add( ".survey" );
	    BufferString survfnm = fp.fullPath();
	    if ( File::exists(survfnm) )
		bss.add( dirnm );
	}
	bss.sort();
    }
    return bss;
}



uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p, CtxtIOObj& ctio )
    : uiSelectFromList(p,uiSelectFromList::Setup("Select Survey",getSurvList()))
    , ctio_(ctio)
{}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    ctio_.setObj( 0 );
}


bool uiSelObjFromOtherSurvey::acceptOK( CallBacker* )
{
    const char* basedir = GetBaseDataDir();
    if ( !basedir ) return false;
    FilePath fp( basedir ); fp.add( selFld()->getText() );
    const BufferString tmprootdir( fp.fullPath() );
    if ( !File::exists(tmprootdir) ) 
	{ uiMSG().error( "Survey doesn't seem to exist" ); return false; }
    const BufferString realrootdir( GetDataDir() );

    IOM().setRootDir( tmprootdir );
    bool prevctiostate = ctio_.ctxt.forread;
    ctio_.ctxt.forread = true;
    uiIOObjSelDlg objdlg( this, ctio_ );
    bool success = false;
    if ( objdlg.go() && objdlg.ioObj() )
    {
	ctio_.setObj( objdlg.ioObj()->clone() );
	ctio_.setName( ctio_.ioobj->name() );
	success = true;
    }
    IOM().setRootDir( realrootdir );
    ctio_.ctxt.forread = prevctiostate;
    return success;
}

