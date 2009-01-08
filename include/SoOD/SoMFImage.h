#ifndef SoMFImage_h
#define SoMFImage_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: SoMFImage.h,v 1.3 2009-01-08 09:48:12 cvsnanne Exp $
________________________________________________________________________

-*/


#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbImage.h>

/*!
The SoMFPlane class is a container for SbImage values.

This field is used where nodes, engines or other field containers needs to store
multiple 2d or 3d images. */ 

class COIN_DLL_API SoMFImage : public SoMField
{
    SO_MFIELD_HEADER( SoMFImage, SbImage, const SbImage& );

public:
    static void		initClass();
};



#endif
