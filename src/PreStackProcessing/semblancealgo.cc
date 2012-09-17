/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* rcsID = "$Id: semblancealgo.cc,v 1.1 2010/11/08 22:06:03 cvskris Exp $";

#include "semblancealgo.h"
#include "ptrman.h"

namespace PreStack
{

mImplFactory( SemblanceAlgorithm, SemblanceAlgorithm::factory );

SemblanceAlgorithm::~SemblanceAlgorithm()
{}

}; //namespace
