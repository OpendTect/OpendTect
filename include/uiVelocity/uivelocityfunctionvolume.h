#ifndef uivelocityfunctionvolume_h
#define uivelocityfunctionvolume_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivelocitymod.h"
#include "uiselectvelocityfunction.h"

#include "velocityfunctionvolume.h"

class uiSeisSel;
class uiGenInput;

namespace Vel
{
class VolumeFunctionSource;

mClass(uiVelocity) uiVolumeFunction : public uiFunctionSettings
{
public:
    mDefaultFactoryInstanciationBase(
	    VolumeFunctionSource::sFactoryKeyword(),
	    VolumeFunctionSource::sFactoryDisplayName() );

    			uiVolumeFunction(uiParent*,VolumeFunctionSource*);
    			~uiVolumeFunction();

    FunctionSource*	getSource();
    bool		acceptOK();

protected:
    static uiFunctionSettings*	create(uiParent*,FunctionSource*);

    uiSeisSel*			volumesel_;
    VolumeFunctionSource*	source_;
};


}; //namespace

#endif

