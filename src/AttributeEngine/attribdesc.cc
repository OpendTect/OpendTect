/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribdesc.cc,v 1.1 2005-01-26 09:15:22 kristofer Exp $";

#include "attribdesc.h"

#include "attribdescset.h"
#include "errh.h"

namespace Attrib
{

Desc::Desc()
    : ds( 0 )
{
    mRefCountConstructor;
}


void Desc::setDescSet( DescSet* nds )
{ ds = nds; }


DescSet* Desc::descSet() const { return ds; }


int Desc::isSatisfied() const { return id()==-1 ? 2 : 0; }


int Desc::id() const { return ds ? ds->getID(*this) : -1; }


const char* Desc::userRef() const { return userref; }


void Desc::setUserRef(const char* nur ) { userref = nur; }

}; //namespace
