/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2010
-*/

static const char* mUnusedVar rcsID = "$Id: semblancealgo.cc,v 1.2 2012-05-02 11:53:21 cvskris Exp $";

#include "semblancealgo.h"
#include "ptrman.h"

namespace PreStack
{

mImplFactory( SemblanceAlgorithm, SemblanceAlgorithm::factory );

SemblanceAlgorithm::~SemblanceAlgorithm()
{}

}; //namespace
