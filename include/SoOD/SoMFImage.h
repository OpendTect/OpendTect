#ifndef SoMFImage_h
#define SoMFImage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id$
________________________________________________________________________

-*/


#include <Inventor/fields/SoSubField.h>

#include "SbImagei32.h"

#include "soodbasic.h"

/*!
The SoMFImagei32 class is a container for SbImagei32 values.

This field is used where nodes, engines or other field containers needs to store
multiple, large 2d or 3d images. */ 

mExportClass( SoOD ) SoMFImagei32 : public SoMField
{
    SO_MFIELD_HEADER( SoMFImagei32, SbImagei32, const SbImagei32& );

public:
    static void		initClass();
};


#endif
