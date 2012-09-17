/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* rcsID = "$Id: uisemblancealgo.cc,v 1.1 2010/11/09 22:23:08 cvskris Exp $";

#include "uisemblancealgo.h"
#include "ptrman.h"

namespace PreStack
{

mImplFactory1Param( uiSemblanceAlgorithm, uiParent*,
		    uiSemblanceAlgorithm::factory);

uiSemblanceAlgorithm::uiSemblanceAlgorithm( uiParent* p, const char* helpid )
    : uiDialog( p,
	  uiDialog::Setup("Setup","Semblance parameters",helpid).canceltext(0))
{}


}; //namespace
