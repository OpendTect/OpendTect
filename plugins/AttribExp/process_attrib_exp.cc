/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: process_attrib_exp.cc,v 1.2 2008-11-25 15:35:21 cvsbert Exp $";

/* process_attrib with experimental attributes */

#include "expfact.h"
#include "prog.h"
#include "../Attrib/process_attrib.cc"


int ExperimentalAttribFactory::id = ExperimentalAttribFactory::addAttribs();
