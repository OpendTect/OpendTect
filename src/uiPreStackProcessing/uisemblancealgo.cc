/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* mUnusedVar rcsID = "$Id: uisemblancealgo.cc,v 1.2 2012-05-02 11:53:50 cvskris Exp $";

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
