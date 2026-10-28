#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "coltabsequence.h"

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec4.h>

namespace vsgGeo { class ColorLookup; }

namespace dGBVis
{

/*!\brief The colors of a ColTab::Sequence as a strip a shader can look up.

  The undefined color is the last pixel of the strip, after the colors, for
  the values that cannot be mapped.
*/

mExpClass(vsgBase) ColTabLookup
{
public:
				ColTabLookup(int nrcolors=255);
				~ColTabLookup();

    void			setSequence(const ColTab::Sequence&);
    const ColTab::Sequence&	sequence() const	{ return sequence_; }

    int				nrColors() const;
    vsg::vec4			colorAt(float pos) const;
				//!<0 <= pos <= 1, as the shader gets it
    vsg::vec4			undefColor() const;

    vsg::ref_ptr<vsgGeo::ColorLookup> colorLookupPtr() const;

private:

    void			update();

    ColTab::Sequence		sequence_;
    vsg::ref_ptr<vsgGeo::ColorLookup> lookup_;
};

} // namespace dGBVis
