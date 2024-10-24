/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uihorizontracksetup.h"


mDefModInitFn(uiMPE)
{
    mIfNotFirstTime( return );

    MPE::uiHorizon2DSetupGroup::initClass();
    MPE::uiHorizon3DSetupGroup::initClass();

    FactoryBase& hor2dsufact = MPE::uiHorizon2DSetupGroupBase::factory();
    if ( StringView(hor2dsufact.getDefaultName()).isEmpty() )
    {
	const int defidx = hor2dsufact.getNames().indexOf(
			MPE::uiHorizon2DSetupGroup::sFactoryKeyword() );
	hor2dsufact.setDefaultName( defidx );
    }

    FactoryBase& hor3dsufact = MPE::uiHorizon3DSetupGroupBase::factory();
    if ( StringView(hor3dsufact.getDefaultName()).isEmpty() )
    {
	const int defidx = hor3dsufact.getNames().indexOf(
			MPE::uiHorizon3DSetupGroup::sFactoryKeyword() );
	hor3dsufact.setDefaultName( defidx );
    }
}
