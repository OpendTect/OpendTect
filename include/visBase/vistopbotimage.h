#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "visobject.h"
#include "coord.h"
#include "multiid.h"

namespace OD{ class RGBImage; }
namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }


namespace visBase
{

mExpClass(visBase) TopBotImage : public VisualObjectImpl
{
public:
    static TopBotImage*		create()
				mCreateDataObj(TopBotImage);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setImageID(const MultiID&);
    MultiID			getImageID() const	{ return odimageid_; }

    void			setPos(const Coord3& tl,const Coord3& br);
    const Coord3&		topLeft() const	    { return pos0_; }
    const Coord3&		bottomRight() const { return pos1_; }

    void			setImageFilename(const char*);
    const char*			getImageFilename() const;
    void			setRGBImageFromFile(const char*);

    void			setTransparency(float); // 0-1
    float			getTransparency() const; // returns value 0-1

    bool			getImageInfo(int& w,int& h,int& pixsz) const;
    const unsigned char*	getTextureData() const;
    bool			getTextureDataInfo(TypeSet<Coord3>& coords,
						   TypeSet<Coord>& texcoords,
						   TypeSet<int>& ps ) const;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
				~TopBotImage();

    void			updateCoords();
    void			setRGBImage(const OD::RGBImage&);


    const mVisTrans*		trans_;
    Coord3			pos0_;
    Coord3			pos1_;
    BufferString		filenm_;
    MultiID			odimageid_;

    static const char*		sKeyTopLeftCoord();
    static const char*		sKeyBottomRightCoord();

    int					layerid_;
    osgGeo::LayeredTexture*		laytex_;
    osgGeo::TexturePlaneNode*		texplane_;
};

} // namespace visBase
