#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgobject.h"
#include "vsggeotextureplane.h"

#include "position.h"
#include "typeset.h"

#include <vsg/core/ref_ptr.h>

namespace dGBVis
{

class TextureChannels;

/*!\brief Counterpart of visBase::TextureRectangle: a rectangle with the
  texture of a TextureChannels.

  The width has one dimension zero. The first dimension with a width is the
  direction of the rows of the data, the second one of the columns, unless
  setRotation tells otherwise. As in visBase the texture is one cell shorter
  than the image, so the corners are at the middle of the first and last cell.
  A plane is made for each tile; they are blended only if the color table has
  transparency.
*/

mExpClass(vsgBase) TextureRectangle : public VisualObjectImpl
{
public:
    static RefMan<TextureRectangle> create();

    void			setTextureChannels(TextureChannels*);
    TextureChannels*		getTextureChannels()
				{ return channels_.ptr(); }

    void			freezeDisplay(bool yn=true);
    bool			isDisplayFrozen() const	{ return frozen_; }
				//!<Postpones the display of changes

    void			setTextureShift(const Coord&);
				//!<Of the texture from the center, in cells
    Coord			getTextureShift() const	{ return shift_; }
    void			setTextureGrowth(const Coord&);
				//!<Of the texture size, in cells
    Coord			getTextureGrowth() const { return growth_; }

    void			setCenter(const Coord3&);
    Coord3			getCenter() const	{ return center_; }
    void			setWidth(const Coord3&);
				//!<One dim must be set zero
    Coord3			getWidth() const	{ return width_; }

    void			setRotation(const Coord3& spanvec0,
					    const Coord3& spanvec1);
    void			setRotationAndWidth(const Coord3& spanvec0,
						    const Coord3& spanvec1);
				//!<Also sets the width to their lengths
    const Coord3&		getSpanningVector(int idx) const
				{ return idx ? spanvec1_ : spanvec0_; }

    void			swapTextureAxes(bool yn=true);
    bool			areTextureAxesSwapped() const
				{ return swapped_; }

    void			setDisplayTransformation(
					const mDGBVisTrans*) override;
    const mDGBVisTrans*		getDisplayTransformation() const override
				{ return displaytrans_.ptr(); }

private:
				TextureRectangle();
				~TextureRectangle();

    void			textureChangedCB(CallBacker*);
    void			colTabChangedCB(CallBacker*);
    void			rebuild();
    Coord3			toDisplay(const Coord3&) const;

    RefMan<TextureChannels>		channels_;
    Coord3				center_;
    Coord3				width_;
    Coord3				spanvec0_	= Coord3( 1., 0., 0. );
    Coord3				spanvec1_	= Coord3( 0., 1., 0. );
    bool				rotated_	= false;
    Coord				shift_		= Coord( 0., 0. );
    Coord				growth_		= Coord( 0., 0. );
    bool				swapped_	= false;
    bool				frozen_		= false;
    bool				needsrebuild_	= false;
    bool				blended_	= false;
    ConstRefMan<mDGBVisTrans>		displaytrans_;
    TypeSet<vsg::ref_ptr<vsgGeo::TexturePlane>>	planes_;
};

} // namespace dGBVis
