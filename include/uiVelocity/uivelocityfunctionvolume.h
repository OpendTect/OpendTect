#ifndef uivelocityfunctionvolume_h
#define uivelocityfunctionvolume_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionvolume.h,v 1.5 2010-11-09 22:19:50 cvskris Exp $
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
	    VolumeFunctionSource::sUserName() );

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
