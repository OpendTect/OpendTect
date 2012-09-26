/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

/* process_attrib with experimental attributes */

#include "expfact.h"
#include "prog.h"
#include "../Attrib/process_attrib.cc"


int ExperimentalAttribFactory::id = ExperimentalAttribFactory::addAttribs();
