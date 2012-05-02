/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* mUnusedVar rcsID = "$Id: pickretriever.cc,v 1.4 2012-05-02 11:53:11 cvskris Exp $";

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
