/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "math2.h"

static ODPresentationManager* prman_ = 0;
ODPresentationManager& ODPrMan()
{
    if ( !prman_ )
	prman_ = new ODPresentationManager;
    return *prman_;
}


ODPresentationManager::ODPresentationManager()
    : triggeredfromdomain_( ODPresentationManager::Scene3D )
{
    syncAllDomains();
}


bool ODPresentationManager::isSyncedWithTriggerDomain(
	DisplayDomain curdomain ) const
{
    return areDomainsSynced( curdomain, triggeredfromdomain_ );
}


bool ODPresentationManager::isTriggeredFromDomain(
	DisplayDomain curdomain ) const
{
    return triggeredfromdomain_ == curdomain;
}


bool ODPresentationManager::areDomainsSynced( DisplayDomain domain1,
					      DisplayDomain domain2 ) const
{
    return Math::AreBitsSet( syncoption_, domain1 ) &&
	   Math::AreBitsSet( syncoption_, domain2 );
}


void ODPresentationManager::syncAllDomains()
{
    syncoption_ = Scene3D | Viewer2D | Basemap;
}
