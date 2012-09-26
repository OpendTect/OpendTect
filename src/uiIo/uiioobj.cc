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
#include "filepath.h"


bool uiIOObj::removeImpl( bool rmentry, bool mustrm )
{
    bool dorm = true;
    if ( !silent_ )
    {
	BufferString mess = "Remove ";
	if ( !rmentry ) mess += "existing ";
	mess += "data file(s), at\n'";
	if ( !ioobj_.isSubdir() )
	{
	    mess += ioobj_.fullUserExpr(true);
	    mess += "'?";
	}
	else
	{
	    BufferString fullexpr( ioobj_.fullUserExpr(true) );
	    mess += FilePath(fullexpr).fileName();
	    mess += "'\n- and everything in it! - ?";
	}
	if ( !uiMSG().askRemove(mess) )
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
	    BufferString mess = "Could not remove data file(s).\n";
	    mess += "Remove entry from list anyway?";
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

    IOM().to( ctio.ctxt.getSelKey() );
    const IOObj* existioobj = (*IOM().dirPtr())[nm];
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	IOM().getEntry( ctio );
	return ctio.ioobj;
    }

    if ( warnifexist )
    {
	BufferString msg( "Overwrite existing '" );
	msg += nm; msg += "'?";
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
    }

    ctio.setObj( existioobj->clone() );
    return true;
}
