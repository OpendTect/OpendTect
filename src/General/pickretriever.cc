/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "pickretriever.h"

RefMan<PickRetriever> PickRetriever::instance_ = 0;

PickRetriever::PickRetriever()	{}
PickRetriever::~PickRetriever()	{}

void PickRetriever::setInstance( PickRetriever* npr )
{ instance_ = npr; }

PickRetriever* PickRetriever::getInstance()
{ return instance_; }

