/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: pickretriever.cc,v 1.3 2009/07/22 16:01:32 cvsbert Exp $";

#include "pickretriever.h"

RefMan<PickRetriever> PickRetriever::instance_ = 0;


PickRetriever::PickRetriever()
{}


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
