/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: process_attrib_exp.cc,v 1.3 2009/07/22 16:01:26 cvsbert Exp $";

/* process_attrib with experimental attributes */

#include "expfact.h"
#include "prog.h"
#include "../Attrib/process_attrib.cc"


int ExperimentalAttribFactory::id = ExperimentalAttribFactory::addAttribs();
