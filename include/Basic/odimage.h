#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "color.h"
#include "ptrman.h"
#include "uistring.h"

namespace OD
{

/*!
\brief Class for Red, Green, Blue image.
*/

mExpClass(Basic) RGBImage
{
public:
    virtual		~RGBImage()			{}

    virtual char	nrComponents() const		= 0;
			/*!<\retval 1 grayscale
			    \retval 2 grayscale+alpha
			    \retval 3 rgb
			    \retval 4 rgb+alpha */
    virtual bool	hasAlpha() const;
    virtual bool	setSize(int,int)		= 0;
    virtual int		getSize(bool xdir) const	= 0;
    virtual Color	get(int,int) const		= 0;
    virtual bool	set(int,int,const Color&)	= 0;
    virtual void	clear(const Color&)		= 0;

    virtual int		bufferSize() const;
    virtual void	fill(unsigned char*) const;
			/*!Fills array with content. Each
			   pixel's components are the fastest
			   dimension, slowest is xdir. Caller
			   must ensure sufficient mem is
			   allocated. */
    virtual bool	put(const unsigned char*,bool xdir_slowest=true,
			    bool with_opacity=false);
			/*!Fills image with data from array.param xdir_slowest
			   False if ydir is the slowest dimension, param
			   with_opacity If true, eventual 4th component will be
			   treated as a opacity instead of a transparency.*/
    virtual bool	blendWith(const RGBImage& sourceimage,
				  bool blendtransparency = false,
				  unsigned char blendtransparencyval = 0,
				  bool blendequaltransparency = false,
				  bool with_opacity=false);
			/*!<Blends image with another image of same size.
			The provided images' transparency will be used to
			blend the two images proportionally.
			\param sourceimage
			\param blendtransparency if true, the color will be
			blended by source image's color if the transparency
			equal to the param blendtransparencyval.
			\param blendtransparencyval
			\param blendequaltransparency if false,the color
			will be not blended when sourceimage has same
			transparency as this image has.
			\param with_opacity if true,eventual 4th component
			will be treated as a opacity instead of a
			transparency.
			*/
    virtual bool	putFromBitmap(const unsigned char* bitmap,
				      const unsigned char* mask = 0);

    virtual const unsigned char*	getData() const		{ return 0; }
    virtual unsigned char*		getData() 		{ return 0; }
};

/*!
\brief Class To load image files, and return the image data in RGBImage object
*/

mExpClass(Basic) RGBImageLoader
{
public:

    static RGBImage*    loadRGBImage(const char* fnm,uiString& errmsg);
			//!<Loads an image from file.
			//!<Should be managed by caller.

private:

    virtual RGBImage*   loadImage(const char*, uiString&) const = 0;
    static  PtrMan<RGBImageLoader>   imageloader_;

public:

    static void		 setImageLoader(RGBImageLoader*);
			 //!<Sets the current imageloader that will load all
			 //!<subsequent images.

    virtual		 ~RGBImageLoader();

};

} // namespace OD
