/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2006
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "transl.h"
#include "filepath.h"


bool uiIOObj::removeImpl( bool rmentry, bool mustrm, bool doconfirm )
{
    bool dorm = true;
    const bool isoutside = !ioobj_.isInCurrentSurvey();
    if ( !silent_ )
    {
	uiString mess = tr("Remove %1data file(s) at\n'%2'%3")
		      .arg(!rmentry ? tr("existing ") 
				    : uiString::emptyString());

	if (!ioobj_.isSubdir())
	{
	    mess.arg(ioobj_.fullUserExpr(true));
	    mess.arg(isoutside ? tr("\nFile not in current survey.\n"
				    "Specify what you would like to remove") 
			       : uiString::emptyString());
	}
	else
	{
	    BufferString fullexpr(ioobj_.fullUserExpr(true));
	    mess.arg(FilePath(fullexpr).fileName());
	    mess.arg(tr("\n- and everything in it! - ?"));
	}

	if ( isoutside )
	{
	    const int resp = uiMSG().question(mess, tr("Remove file"),
					      tr("Remove link"), tr("Cancel"),
					      tr("Remove data"));
	    if ( resp < 0 )
		return false;
	    dorm = resp;
	}
	else if ( doconfirm && !uiMSG().askRemove(mess) )
	{
	    if ( mustrm )
		return false;
	    dorm = false;
	}
    }

    if ( dorm && !fullImplRemove(ioobj_) )
    {
	if ( !silent_ )
	{
	    uiString mess = tr("Could not remove data file(s).\n"
			       "Remove entry from list anyway?");
	    if ( !uiMSG().askRemove(mess) )
		return false;
	}
    }

    if ( rmentry )
	IOM().permRemove( ioobj_.key() );

    return true;
}


bool uiIOObj::fillCtio( CtxtIOObj& ctio, bool warnifexist )
{
    if ( ctio.name().isEmpty() )
    {
	if ( !ctio.ioobj )
	    return false;
	ctio.setName( ctio.ioobj->name() );
    }
    const char* nm = ctio.name().buf();

    const IODir iodir( ctio.ctxt.getSelKey() );
    const IOObj* existioobj = iodir.get( nm, ctio.ctxt.trgroup->userName() );
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	IOM().getEntry( ctio );
	return ctio.ioobj;
    }

    if ( warnifexist )
    {
	uiString msg = tr("Overwrite existing '%1'?").arg(nm);
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
    }

    ctio.setObj( existioobj->clone() );
    return true;
}
