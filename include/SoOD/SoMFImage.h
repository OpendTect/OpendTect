#ifndef SoMFImage_h
#define SoMFImage_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: SoMFImage.h,v 1.2 2009-01-08 09:27:06 cvsranojay Exp $
________________________________________________________________________

-*/


#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbImage.h>

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
