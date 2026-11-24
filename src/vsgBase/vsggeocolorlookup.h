#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/Array2D.h>
#include <vsg/core/Inherit.h>
#include <vsg/core/Object.h>
#include <vsg/maths/vec4.h>
#include <vsg/state/DescriptorImage.h>

namespace vsgGeo
{

/*!\brief A strip of colors that a shader looks values up in.

  The strip is a one-pixel high texture. The shader reads the color of an
  index from the pixel with that number. One more pixel after the colors holds
  the color for the values that are undefined.

  setColors changes the strip in place; the new colors are used from the
  next frame on. The number of colors is fixed when the strip is made.
*/

class VSGGEO_EXPORT ColorLookup : public vsg::Inherit<vsg::Object,ColorLookup>
{
public:

				ColorLookup(int nrcolors=255);

    int				nrColors() const;
				//!<Without the undefined color
    bool			setColors(const vsg::vec4* colors,int nr,
					  const vsg::vec4& undefcolor);
				//!<False if nr is not nrColors()
    vsg::vec4			colorAt(float pos) const;
				//!<The color at 0 <= pos <= 1 along the strip
    vsg::vec4			undefColor() const;

    vsg::ref_ptr<vsg::DescriptorImage> descriptor(uint32_t binding) const;

protected:
				~ColorLookup();

private:

    vsg::ref_ptr<vsg::ubvec4Array2D>	strip_;
    vsg::ref_ptr<vsg::Sampler>		sampler_;
};

} // namespace vsgGeo
