/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpsdemo.cc,v 1.1 2009-11-04 09:52:51 cvsbert Exp $";

#include "uidpsdemo.h"

#include "uilabel.h"


uiDPSDemo::uiDPSDemo( uiParent* p )
	: uiDialog(p,Setup("DataPointSet demo","Data extraction parameters",
		    	   mNoHelpID))
{
    new uiLabel( this, "TODO: implement" );
}
