#ifndef vistexturechannel2rgba_h
#define vistexturechannel2rgba_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vistexturechannel2rgba.h,v 1.1 2008-10-09 21:45:23 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoColTabTextureChannel2RGBA;
class SbImage;
class SoSwitch;
class SoGroup;
class SoTextureUnit;
class SoTexture2;
class SoShaderProgram;
class SoFragmentShader;
class SoShaderParameter1i;
class SoShaderParameterArray1f;

namespace ColTab { class Sequence; }

namespace visBase
{ 

class TextureChannels;
/*!\brief


*/

class TextureChannel2RGBA : public DataObject
{
public:
    virtual void		setChannels(TextureChannels*);
    virtual bool		createRGBA(SbImage&) const		= 0;
    virtual bool		useShading(bool)			= 0;
    virtual bool		usesShading() const			= 0;
    virtual int			maxNrChannels() const			= 0;

    virtual bool		canUseShading() const			= 0;
protected:
    				TextureChannel2RGBA();

    TextureChannels*		channels_;
};


class ColTabTextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static ColTabTextureChannel2RGBA*	create()
			mCreateDataObj(ColTabTextureChannel2RGBA);

    virtual void	setSequence( int channel, const ColTab::Sequence& );


    bool		canUseShading() const;
    bool		useShading(bool);
    bool		usesShading() const;
    int			maxNrChannels() const;

    bool		createRGBA(SbImage&) const;

protected:
    					~ColTabTextureChannel2RGBA();
    SoNode*				getInventorNode();

    void				update();
    void				getColors(int channel,
					      TypeSet<unsigned char>&) const;

    TypeSet<ColTab::Sequence>		coltabs_;
    BoolTypeSet				enabled_;
    TypeSet<unsigned char>		opacity_;

    SoSwitch*				shaderswitch_;

    					//Shading stuff
    void				setShadingVars();
    void				createFragShadingProgram(int nrchannesl,
	    					BufferString&) const;
    bool				hasTransparency(int channel) const;
    static const char*			sVertexShaderProgram();
    SoGroup*				shadinggroup_;
    SoTexture2*				shaderctab_;
    SoFragmentShader*			fragmentshader_;
    SoShaderParameter1i*		numlayers_;
    SoShaderParameter1i*		startlayer_;
    SoShaderParameterArray1f*		layeropacity_;

					//Non shading
    void				doFill(
	    				    SoColTabTextureChannel2RGBA*) const;
    SoColTabTextureChannel2RGBA*	converter_;

};

}; //namespace


#endif
