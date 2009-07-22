#ifndef uivelocityfunctionvolume_h
#define uivelocityfunctionvolume_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionvolume.h,v 1.4 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________


-*/

#include "uiselectvelocityfunction.h"

class uiSeisSel;
class uiGenInput;

namespace Vel
{
class VolumeFunctionSource;

mClass uiVolumeFunction : public uiFunctionSettings
{
public:
    static void		initClass();

    			uiVolumeFunction(uiParent*,VolumeFunctionSource*);
    			~uiVolumeFunction();

    FunctionSource*	getSource();
    bool		acceptOK();

protected:
    static uiFunctionSettings*
				create(uiParent*,FunctionSource*);

    uiSeisSel*			volumesel_;
    VolumeFunctionSource*	source_;
};


}; //namespace

#endif
