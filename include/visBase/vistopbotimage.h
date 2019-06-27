#ifndef vistopbotimage_h
#define	vistopbotimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		June 2009
________________________________________________________________________

-*/


#include "visobject.h"
#include "position.h"

namespace OD{ class RGBImage; }
namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }


namespace visBase
{


mExpClass(visBase) TopBotImage : public VisualObjectImpl
{
public:
    static TopBotImage*		create()
				mCreateDataObj(TopBotImage);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
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

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
				~TopBotImage();

    void			updateCoords();
    void			setRGBImage(const OD::RGBImage&);


    const mVisTrans*		trans_;
    Coord3			pos0_;
    Coord3			pos1_;
    BufferString		filenm_;

    static const char*		sKeyTopLeftCoord();
    static const char*		sKeyBottomRightCoord();
    static const char*		sKeyFileNameStr();
    static const char*		sKeyTransparencyStr();

    int					layerid_;
    osgGeo::LayeredTexture*		laytex_;
    osgGeo::TexturePlaneNode*		texplane_;
};

} // namespace visBase

#endif
