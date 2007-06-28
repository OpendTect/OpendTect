
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadbldcmd.cc,v 1.2 2007-06-28 18:11:32 cvsbert Exp $";

#include "uimadbldcmd.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uiexecutor.h"
#include "maddefs.h"
#include "executor.h"


uiMadagascarBldCmd::uiMadagascarBldCmd( uiParent* p, BufferString& cmd )
	: uiDialog( p, Setup( "Madagascar command building",
			      "Build Madagascar command",
			      "0.0.0") )
	, cmd_(cmd)
{
    bool allok = ODMad::PI().errMsg().isEmpty();
    if ( allok && !ODMad::PI().scanned() )
    {
	Executor* ex = ODMad::PI().getScanner();
	uiExecutor dlg( this, *ex );
	allok = dlg.go() && ODMad::PI().defs().size() > 0;
	delete ex;
    }

    uiSeparator* sep;
    if ( allok )
	sep = createMainPart();
    else
    {
	uiLabel* lbl = new uiLabel( this, ODMad::PI().errMsg() );
	sep = new uiSeparator( this, "low sep" );
	sep->attach( stretchedBelow, lbl );
    }

    cmdfld_ = new uiGenInput( this, "Command line", cmd );
    cmdfld_->attach( centeredBelow, sep );
}


uiMadagascarBldCmd::~uiMadagascarBldCmd()
{
}


uiSeparator* uiMadagascarBldCmd::createMainPart()
{
    BufferString msg( "Found " ); msg += ODMad::PI().defs().size();
    msg += " in "; msg += ODMad::PI().groups().size(); msg += " groups.";
    uiLabel* lbl = new uiLabel( this, msg );
    uiSeparator* sep = new uiSeparator( this, "low sep" );
    sep->attach( stretchedBelow, lbl );
    return sep;
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
