#ifndef openclplatform_h
#define openclplatform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2007
________________________________________________________________________

-*/

#include "openclmod.h"
#include "commondefs.h"
#include "bufstring.h"

template <class T> class ObjectSet;


namespace OpenCL
{
    mExpClass(OpenCL) Platform
    {
    public:
        static void				initClass();
        static const ObjectSet<Platform>&	getPlatforms();
    
        BufferString                            vendor_;
        BufferString                            name_;
    };

}  //namespace


#endif
