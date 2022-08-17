#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
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
