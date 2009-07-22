#ifndef SoMFImage_h
#define SoMFImage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: SoMFImage.h,v 1.5 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________

-*/


#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbImage.h>

#include "soodbasic.h"

/*!
The SoMFPlane class is a container for SbImage values.

This field is used where nodes, engines or other field containers needs to store
multiple 2d or 3d images. */ 

mClass SoMFImage : public SoMField
{
    SO_MFIELD_HEADER( SoMFImage, SbImage, const SbImage& );

public:
    static void		initClass();
};



#endif
