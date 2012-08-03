#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistexturerect.h,v 1.38 2012-08-03 13:01:27 cvskris Exp $
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "visobject.h"

class CubeSampling;

namespace osgGeo { class TexturePlaneNode; }

namespace visBase
{

class TextureChannels;

/*!\brief
    A TextureRectangle is a Rectangle with a datatexture.  The data is set via setData.
*/

mClass(visBase) TextureRectangle : public VisualObjectImpl
{
public:
    static TextureRectangle*	create()
				mCreateDataObj(TextureRectangle);

    void			setTextureChannels(visBase::TextureChannels*);
    void			setCenter(const Coord3& center);
    void			setWidth(const Coord3& width);
    				//!<One dim must be 0
    Coord3			getWidth() const;
    Coord3			getCenter() const;

    void			swapTextureAxes(bool yn=true);
    bool			areTextureAxesSwapped() const;

protected:
    				~TextureRectangle();

    osgGeo::TexturePlaneNode*	textureplane_;
    RefMan<TextureChannels>	channels_;
};

};

#endif

