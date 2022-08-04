#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Jaap Glas
 Date:		April 2013
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "visobject.h"
#include "vistransform.h"


namespace osgGeo { class TexturePanelStripNode; }

namespace visBase
{

class TextureChannels;

/*!\brief
    A TexturePanelStrip is geometrically invariant in the z-direction. Its
    serially connected panels are painted with a layered texture.
*/

mExpClass(visBase) TexturePanelStrip : public VisualObjectImpl
{
public:
    static TexturePanelStrip*	create()
				mCreateDataObj(TexturePanelStrip);

    void			setTextureChannels(visBase::TextureChannels*);
    visBase::TextureChannels*	getTextureChannels();

    void			freezeDisplay(bool yn=true);
    bool			isDisplayFrozen() const;
				/*!<As long as texture panel strip is frozen,
				    the display of (lengthy) changes to its
				    geometry and/or texture is postponed.
				    Avoids showing half-finished updates. */

    void			setPath(const TypeSet<Coord>&);
    const TypeSet<Coord>&	getPath() const;

    void			setPath2TextureMapping(
						const TypeSet<float>& offsets);
				/*!<Monotonously non-decreasing list of column
				    offsets into the texture. There should be
				    one offset per path coordinate. */
    const TypeSet<float>&	getPath2TextureMapping() const;

    void			setPathTextureShift(float shift,int startidx=0);
				/*!<Extra horizontal shift of (part of) the
				    specified texture mapping (in pixel units).
				    Shifting starts at path coord "startidx" */
    float			getPathTextureShift() const;
    float			getPathTextureShiftStartIdx() const;

    void			setZRange(const Interval<float>&);
    Interval<float>		getZRange() const;

    void			unsetZRange2TextureMapping();
    void			setZRange2TextureMapping(
						const Interval<float>& offsets);
				//!<Mapped on full texture row range if not set.
    bool			isZRange2TextureMappingSet() const;
    Interval<float>		getZRange2TextureMapping() const;

    void			setZTextureShift(float);
				/*!<Extra vertical shift of the specified
				    texture mapping (in pixel units). */
    float			getZTextureShift() const;

    void			swapTextureAxes(bool yn=true);
    bool			areTextureAxesSwapped() const;

    void			smoothNormals(bool yn=true);
    bool			areNormalsSmoothed() const;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;
    int				getNrTextures() const;
    const unsigned char*	getTextureData() const;
    mStruct(visBase) TextureDataInfo
    {
	TypeSet<Coord3> coords_;
	TypeSet<Coord>	texcoords_;
	TypeSet<int>	ps_;
	void		setEmpty() { coords_.erase(); texcoords_.erase();
				     ps_.erase(); }
    };

    bool			getTextureDataInfo(int tidx,
				    TextureDataInfo& texinfo) const;
    bool			getTextureInfo(int& width,int& height,
				    int& pixsize);
protected:
					~TexturePanelStrip();
    void				updatePath();

    ConstRefMan<mVisTrans>		displaytrans_;
    osgGeo::TexturePanelStripNode*	osgpanelstrip_;
    RefMan<TextureChannels>		channels_;
    PtrMan<TypeSet<Coord> >		pathcoords_;
    PtrMan<TypeSet<float> >		pathtexoffsets_;

private:
    bool			calcTextureCoordinates(int,
						       TypeSet<Coord>&)const;
};

} // namespace visBase
