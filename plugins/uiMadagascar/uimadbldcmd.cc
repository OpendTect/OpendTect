
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadbldcmd.cc,v 1.1 2007-06-18 16:39:49 cvsbert Exp $";

#include "uimadbldcmd.h"
#include "uimsg.h"
#include "uigeninput.h"


uiMadagascarBldCmd::uiMadagascarBldCmd( uiParent* p, BufferString& cmd )
	: uiDialog( p, Setup( "Madagascar command building",
			      "Build Madagascar command",
			      "0.0.0") )
	, cmd_(cmd)
{
    cmdfld_ = new uiGenInput( this, "Command line", cmd );
}


uiMadagascarBldCmd::~uiMadagascarBldCmd()
{
}

bool uiMadagascarBldCmd::acceptOK( CallBacker* )
{
    BufferString newcmd = cmdfld_->text();
    if ( newcmd.isEmpty() )
    {
	uiMSG().error( "Please specify a command" );
	return false;
    }

    cmd_ = newcmd;
    return true;
}
