#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "vistransform.h"


/*!\brief A collection of geoscientific extensions to OpenSceneGraph.*/

namespace osgGeo { class TexturePlaneNode; }

namespace visBase
{

class TextureChannels;

/*!\brief A TextureRectangle is a Rectangle with a datatexture.  The data is
    set via setData.
*/

mExpClass(visBase) TextureRectangle : public VisualObjectImpl
{
public:
    static TextureRectangle*	create()
				mCreateDataObj(TextureRectangle);

    void			setTextureChannels(visBase::TextureChannels*);
    visBase::TextureChannels*	getTextureChannels();

    void			freezeDisplay(bool yn=true);
    bool			isDisplayFrozen() const;
				/*!<As long as texture rectangle is frozen,
				    the display of (lengthy) changes to its
				    geometry and/or texture is postponed.
				    Avoids showing half-finished updates. */

    void			setTextureShift(const Coord&);
				/*!<Shift of the texture envelope center
				    (in pixel units) with regard to the
				    center of the plane geometry. */
    Coord			getTextureShift() const;

    void			setTextureGrowth(const Coord&);
				/*!<Size increase/decrease of the texture
				    envelope (in pixel units). Its default
				    size is one pixel shorter than the image
				    envelope (half-a-pixel at every border). */
    Coord			getTextureGrowth() const;

    void			setCenter(const Coord3& center);
    void			setWidth(const Coord3& width);
				//!<One dim must be set zero
    Coord3			getWidth() const;
    Coord3			getCenter() const;

    void			setRotation(const Coord3& spanvec0,
					    const Coord3& spanvec1);
				//!<Aligns rectangle to both spanning vectors
    void			setRotationAndWidth(const Coord3& spanvec0,
						    const Coord3& spanvec1);
				/*!<Aligns rectangle to both spanning vectors,
				  and calls setWidth(.) using their lengths. */
    const Coord3&		getSpanningVector(int idx) const;

    void			swapTextureAxes(bool yn=true);
    bool			areTextureAxesSwapped() const;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    // deprecated
    void			getTextureCoordinates(TypeSet<Coord3>&) const;

protected:
				~TextureRectangle();

    ConstRefMan<mVisTrans>	displaytrans_;

    osgGeo::TexturePlaneNode*	textureplane_;
    RefMan<TextureChannels>	channels_;

    Coord3			spanvec0_;
    Coord3			spanvec1_;

public:

    int				getNrTextures() const;
    const unsigned char*	getTextureData() const;

    mStruct(visBase) TextureDataInfo
    {
	TypeSet<Coord3> coords_;
	TypeSet<Coord>	texcoords_;
	TypeSet<int>	ps_;
	void		setEmpty()	{ coords_.erase(); texcoords_.erase();
					  ps_.erase(); }
    };

    bool			getTextureDataInfo(int tidx,
					    TextureDataInfo& texinfo) const;
    bool			getTextureInfo(int& width,int& height,
					       int& pixsize) const;
};

} // namespace visBase
