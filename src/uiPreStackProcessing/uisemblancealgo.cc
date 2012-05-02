/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* rcsID mUnusedVar = "$Id: uisemblancealgo.cc,v 1.3 2012-05-02 15:12:14 cvskris Exp $";

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
