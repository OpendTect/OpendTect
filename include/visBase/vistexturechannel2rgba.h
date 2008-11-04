#ifndef vistexturechannel2rgba_h
#define vistexturechannel2rgba_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Sep 2008
 RCS:		$Id: vistexturechannel2rgba.h,v 1.7 2008-11-04 21:18:01 cvskris Exp $
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
class SoRGBATextureChannel2RGBA;
class SoTextureComposerInfo;

namespace ColTab { class Sequence; }

namespace visBase
{ 

class TextureChannels;
/*!

Interface for classes that can take one or more texture channels in an
TextureChannels class and convert them to RGBA textures in OpenGL, optionally
with shaders. There should always be a non-shading way to fall back on.
*/

class TextureChannel2RGBA : public DataObject
{
public:
    virtual void		setChannels(TextureChannels*);
    virtual bool		createRGBA(SbImage&) const		= 0;
				/*!<Fill the image with the output, using
				    current settings. */

    virtual void		setEnabled(int ch,bool yn)	{}
    virtual bool		isEnabled(int ch) const		{ return true; }

    virtual bool		canUseShading() const			= 0;
    virtual void		allowShading(bool);
    virtual bool		usesShading() const			= 0;
    virtual int			maxNrChannels() const			= 0;
    virtual int			minNrChannels() const		{ return 1; }

protected:
    				TextureChannel2RGBA();

    TextureChannels*		channels_;
    bool			shadingallowed_;
};


/*! Implementation of TextureChannel2RGBA that takes a ColorSequence for each
channel and blends it into an RGBA image. */


class ColTabTextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static ColTabTextureChannel2RGBA*	create()
				mCreateDataObj(ColTabTextureChannel2RGBA);

    void			setSequence(int ch,const ColTab::Sequence&);
    const ColTab::Sequence&	getSequence(int ch) const;

    void			setEnabled(int ch,bool yn);
    bool			isEnabled(int ch) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    bool			canUseShading() const;
    void			allowShading(bool);
    bool			usesShading() const;
    int				maxNrChannels() const;

    bool			createRGBA(SbImage&) const;

protected:
    void			adjustNrChannels() const;
    void			setChannels(TextureChannels*);

    					~ColTabTextureChannel2RGBA();
    SoNode*				getInventorNode();

    void				update();
    void				getColors(int channel,
					      TypeSet<unsigned char>&) const;

    mutable TypeSet<ColTab::Sequence>	coltabs_;
    mutable BoolTypeSet			enabled_;
    mutable TypeSet<unsigned char>	opacity_;

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
    SoTextureComposerInfo*		tci_;

					//Non shading
    void				doFill(
	    				    SoColTabTextureChannel2RGBA*) const;
    SoColTabTextureChannel2RGBA*	converter_;
};


}; //namespace


#endif
