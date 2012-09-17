#ifndef uivelocityfunctionvolume_h
#define uivelocityfunctionvolume_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionvolume.h,v 1.6 2010/11/10 19:53:12 cvskris Exp $
________________________________________________________________________


-*/

#include "uiselectvelocityfunction.h"

#include "velocityfunctionvolume.h"

class uiSeisSel;
class uiGenInput;

namespace Vel
{
class VolumeFunctionSource;

mClass uiVolumeFunction : public uiFunctionSettings
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
