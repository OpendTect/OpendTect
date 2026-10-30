#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include "vsggeocolorlookup.h"

#include <vsg/core/Array2D.h>
#include <vsg/core/Inherit.h>
#include <vsg/core/Object.h>
#include <vsg/state/DescriptorImage.h>

namespace vsgGeo
{

/*!\brief A layer of color indices, shown with a ColorLookup.

  Like osgGeo::LayeredTexture it keeps the data as bytes and leaves the
  coloring to the shader. A cell is two bands: the color index and a band
  that is 255 if the value is defined. The index is stored multiplied by that
  band and divided out again in the shader, so that interpolation does not mix
  undefined cells into the colors around them.

  One layer for now. Size and interpolation are fixed when it is made.
*/

class VSGGEO_EXPORT LayeredTexture
    : public vsg::Inherit<vsg::Object,LayeredTexture>
{
public:

				LayeredTexture(int width,int height,
					vsg::ref_ptr<ColorLookup> =nullptr,
					bool interpolate=false);

    int				width() const;
    int				height() const;
    bool			isInterpolated() const	{ return interpolated_;}

    static constexpr int	nrBands()		{ return 2; }

    static int			maxTextureSize();
				//!<Width and height; 4096 until set for the device
    static void			setMaxTextureSize(int);

    bool			setIndices(const unsigned char*,size_t size);
				//!<Per cell index and defined band
    unsigned char		indexAt(int x,int y) const;
				//!<0 for an undefined cell
    bool			isUndefAt(int x,int y) const;

    ColorLookup&		colorLookup()	{ return *lookup_; }

    vsg::ref_ptr<vsg::DescriptorImage> indexDescriptor(
						uint32_t binding) const;
    vsg::ref_ptr<vsg::DescriptorImage> colorDescriptor(
						uint32_t binding) const;

protected:
				~LayeredTexture();

private:

    vsg::ref_ptr<vsg::ubvec2Array2D>	image_;
    vsg::ref_ptr<vsg::Sampler>		sampler_;
    vsg::ref_ptr<ColorLookup>		lookup_;
    bool				interpolated_;
};

} // namespace vsgGeo
