#ifndef odpresentationmgr_h
#define odpresentationmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "commondefs.h"


mExpClass(General) ODPresentationManager
{
public:
    enum DisplayDomain		{ Scene3D=0x0001, Viewer2D=0x0002,
				  Basemap=0x0004 };
				ODPresentationManager();

    void			syncAllDomains();
    void			setSyncOption( int opt )
				{ syncoption_ = opt; }
    bool			areDomainsSynced(DisplayDomain domain1,
						 DisplayDomain domain2) const;
    bool			isSyncedWithTriggerDomain(DisplayDomain) const;
    bool			isTriggeredFromDomain(DisplayDomain) const;
    void			setTriggerFromDomain( DisplayDomain opt )
				{ triggeredfromdomain_ = opt; }
protected:
    unsigned int		syncoption_;
    DisplayDomain		triggeredfromdomain_;
};

mGlobal(General) ODPresentationManager& ODPrMan();
#endif
