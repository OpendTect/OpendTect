#ifndef odimage_h
#define odimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2010
 RCS:           $Id: odimage.h,v 1.2 2010-08-09 19:58:41 cvskris Exp $
________________________________________________________________________

-*/

#include "color.h"

namespace OD
{

mClass RGBImage
{
public:
    virtual		~RGBImage()				{}

    virtual char	nrComponents() const			= 0;
    			/*!<\retval 1 grayscale
			    \retval 2 grayscale+alpha
			    \retval 3 rgb
			    \retval 4 rgb+alpha */
    virtual bool	hasAlpha() const;
    virtual bool	setSize(int,int)			= 0;
    virtual int		getSize(bool xdir) const		= 0;
    virtual Color	get(int,int) const			= 0;
    virtual bool	set(int,int,const Color&)		= 0;

    virtual void	fill(unsigned char*) const;
    			/*!Fills array with content. Each pixel's components are
			   the fastest dimension, slowet is xdir. Caller must
			   ensure sufficient mem is allocated. */
};

};

#endif
