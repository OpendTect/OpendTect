#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "uiselectvelocityfunction.h"

#include "velocityfunctionvolume.h"

class uiSeisSel;

namespace Vel
{
class VolumeFunctionSource;

mExpClass(uiVelocity) uiVolumeFunction : public uiFunctionSettings
{
public:
    mDefaultFactoryInstanciationBase(
	    VolumeFunctionSource::sFactoryKeyword(),
	    VolumeFunctionSource::sFactoryDisplayName() );

    			uiVolumeFunction(uiParent*,VolumeFunctionSource*);
    			~uiVolumeFunction();

    FunctionSource*	getSource() override;
    bool		acceptOK() override;

protected:
    static uiFunctionSettings*	create(uiParent*,FunctionSource*);

    uiSeisSel*			volumesel_;
    VolumeFunctionSource*	source_;
};

} // namespace Vel
