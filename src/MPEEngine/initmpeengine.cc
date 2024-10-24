/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "horizon2dextender.h"
#include "horizon3dextender.h"
#include "mpesetup.h"

mDefModInitFn(MPEEngine)
{
    mIfNotFirstTime( return );

    MPESetupTranslatorGroup::initClass();
    dgbMPESetupTranslator::initClass();

    MPE::Horizon2DExtender::initClass();
    MPE::Horizon3DExtender::initClass();

    FactoryBase& hor2dextfact = MPE::Horizon2DExtenderBase::factory();
    if ( StringView(hor2dextfact.getDefaultName()).isEmpty() )
    {
	const int defidx = hor2dextfact.getNames().indexOf(
			MPE::Horizon2DExtender::sFactoryKeyword() );
	hor2dextfact.setDefaultName( defidx );
    }

    FactoryBase& hor3dextfact = MPE::Horizon3DExtenderBase::factory();
    if ( StringView(hor3dextfact.getDefaultName()).isEmpty() )
    {
	const int defidx = hor3dextfact.getNames().indexOf(
			MPE::Horizon3DExtender::sFactoryKeyword() );
	hor3dextfact.setDefaultName( defidx );
    }
}
