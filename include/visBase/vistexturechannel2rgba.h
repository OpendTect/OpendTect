#ifndef vistexturechannel2rgba_h
#define vistexturechannel2rgba_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Sep 2008
 RCS:		$Id: vistexturechannel2rgba.h,v 1.16 2009-06-11 17:14:36 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "coltabsequence.h"

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

namespace visBase
{ 

class TextureChannels;
/*!

Interface for classes that can take one or more texture channels in an
TextureChannels class and convert them to RGBA textures in OpenGL, optionally
with shaders. There should always be a non-shading way to fall back on.
*/

mClass TextureChannel2RGBA : public DataObject
{
public:
    virtual void	setChannels(TextureChannels*);
    virtual void	notifyChannelChange()			{}
    virtual bool	createRGBA(SbImage&) const		= 0;
			/*!<Fill the image with the output, using
			    current settings. */

    virtual bool	canSetSequence() const			{ return false;}
    virtual void	setSequence(int,const ColTab::Sequence&){}
    virtual const ColTab::Sequence* getSequence(int) const	{ return 0; }

    virtual void	swapChannels(int ch0,int ch1)		{}
    virtual void	setEnabled(int ch,bool yn)		{}
    virtual bool	isEnabled(int ch) const			{ return true; }

    virtual bool	canUseShading() const;
    virtual void	allowShading(bool);
    virtual bool	usesShading() const			= 0;
    virtual int		maxNrChannels() const			= 0;
    virtual int		minNrChannels() const			{ return 1; }

protected:
    			TextureChannel2RGBA();

    TextureChannels*	channels_;
    bool		shadingallowed_;
};


/*! Implementation of TextureChannel2RGBA that takes a ColorSequence for each
channel and blends it into an RGBA image. */


mClass ColTabTextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static ColTabTextureChannel2RGBA*	create()
				mCreateDataObj(ColTabTextureChannel2RGBA);

    void			swapChannels(int ch0,int ch1);

    bool			canSetSequence() const		{ return true;}
    void			setSequence(int ch,const ColTab::Sequence&);
    const ColTab::Sequence*	getSequence(int ch) const;

    void			setEnabled(int ch,bool yn);
    bool			isEnabled(int ch) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    void			allowShading(bool);
    bool			usesShading() const;
    int				maxNrChannels() const;

    bool			createRGBA(SbImage&) const;

    bool			canUseShading() const;
protected:
    void			notifyChannelChange()	{ update(); }
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
    char				getTextureTransparency(int ch) const;
    					//See SoTextureComposerInfo for retvals
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
