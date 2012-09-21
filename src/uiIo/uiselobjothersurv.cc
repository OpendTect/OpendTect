/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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

	    const FilePath fp( basedir, dirnm, ".survey" );
	    if ( File::exists(fp.fullPath()) )
		bss.add( dirnm );
	}
	bss.sort();
    }
    return bss;
}



uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p, CtxtIOObj& ctio )
    : uiSelectFromList(p,uiSelectFromList::Setup("Select Survey",getSurvList()))
    , ctio_(ctio)		 
{
    othersurveyrootdir_.setEmpty();
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    ctio_.setObj( 0 );
    setDirToCurrentSurvey();
}


bool uiSelObjFromOtherSurvey::acceptOK( CallBacker* )
{
    const char* basedir = GetBaseDataDir();
    if ( !basedir ) return false;
    othersurveyrootdir_ = FilePath(basedir,selFld()->getText()).fullPath();
    if ( !File::exists( othersurveyrootdir_ ) ) 
    { 
	othersurveyrootdir_.setEmpty();
	uiMSG().error( "Survey doesn't seem to exist" ); 
	return false; 
    }

    setDirToOtherSurvey();
    bool prevctiostate = ctio_.ctxt.forread;
    ctio_.ctxt.forread = true;
    uiIOObjSelDlg objdlg( this, ctio_ );
    bool success = false;
    if ( objdlg.go() && objdlg.ioObj() )
    {
	ctio_.setObj( objdlg.ioObj()->clone() );
	ctio_.setName( ctio_.ioobj->name() );
	fulluserexpression_ = ctio_.ioobj->fullUserExpr();
	success = true;
    }
    ctio_.ctxt.forread = prevctiostate;
    return success;
}


void uiSelObjFromOtherSurvey::setDirToCurrentSurvey()
{
    IOM().setRootDir( GetDataDir() );
}


void uiSelObjFromOtherSurvey::setDirToOtherSurvey()
{
    if ( !othersurveyrootdir_.isEmpty() )
	IOM().setRootDir( othersurveyrootdir_ );
}
