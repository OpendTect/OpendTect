/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: pickretriever.cc,v 1.1 2006-08-26 15:48:10 cvskris Exp $";

#include "pickretriever.h"

RefMan<PickRetriever> PickRetriever::instance_ = 0;


PickRetriever::PickRetriever()
{ mRefCountConstructor; }


PickRetriever::~PickRetriever()
{}


void PickRetriever::setInstance( PickRetriever* npr )
{
    instance_ = npr;
}


PickRetriever* PickRetriever::getInstance()
{
    return instance_;
}
