/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.1 2002-04-21 15:06:56 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "seismmjobman.h"


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm, const IOPar& iopar,
			    const char* seisoutkey, const char* ickey )
	: uiDialog(p,uiDialog::Setup("Multi-machine processing",
				     "Manage processing",
				     "0")
		.canceltext("Quit"))
	, jm(new SeisMMJobMan(prognm,iopar,seisoutkey,ickey))
{
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete jm;
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    return true;
}
