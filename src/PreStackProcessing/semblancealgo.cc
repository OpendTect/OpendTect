/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* rcsID mUnusedVar = "$Id: semblancealgo.cc,v 1.3 2012-05-02 15:11:45 cvskris Exp $";

#include "semblancealgo.h"
#include "ptrman.h"

namespace PreStack
{

mImplFactory( SemblanceAlgorithm, SemblanceAlgorithm::factory );

SemblanceAlgorithm::~SemblanceAlgorithm()
{}

}; //namespace
