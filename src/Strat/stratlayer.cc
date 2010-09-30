/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID = "$Id: stratlayer.cc,v 1.2 2010-09-30 10:58:10 cvsbert Exp $";

#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratunitref.h"


const Strat::LeafUnitRef& Strat::Layer::unitRef() const
{
    return ref_ ? *ref_ : LeafUnitRef::undef();
}


Strat::Layer::ID Strat::Layer::id() const
{
    return unitRef().fullCode();
}
