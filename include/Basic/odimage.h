#ifndef odimage_h
#define odimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2010
 RCS:           $Id: odimage.h,v 1.4 2012-08-03 13:00:13 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "color.h"

namespace OD
{

mClass(Basic) RGBImage
{
public:
    virtual				~RGBImage()			{}

    virtual char			nrComponents() const		= 0;
					/*!<\retval 1 grayscale
					    \retval 2 grayscale+alpha
					    \retval 3 rgb
					    \retval 4 rgb+alpha */
    virtual bool			hasAlpha() const;
    virtual bool			setSize(int,int)		= 0;
    virtual int				getSize(bool xdir) const	= 0;
    virtual Color			get(int,int) const		= 0;
    virtual bool			set(int,int,const Color&)	= 0;

    virtual int				bufferSize() const;
    virtual void			fill(unsigned char*) const;
					/*!Fills array with content. Each
					   pixel's components are the fastest
					   dimension, slowet is xdir. Caller
					   must ensure sufficient mem is
					   allocated. */
    virtual bool			put(const unsigned char*);
    virtual const unsigned char*	getData() const		{ return 0; }
    virtual unsigned char*		getData() 		{ return 0; }
};

};

#endif

