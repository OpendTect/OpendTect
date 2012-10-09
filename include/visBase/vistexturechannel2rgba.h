#ifndef vistexturechannel2rgba_h
#define vistexturechannel2rgba_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visdata.h"

class SoColTabTextureChannel2RGBA;
class SoComplexity;
class SbImagei32;
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
class MappedTextureDataSet;

/*!
Interface for classes that can take one or more texture channels in an
TextureChannels class and convert them to RGBA textures in OpenGL, optionally
with shaders. There should always be a non-shading way to fall back on.
*/

mClass TextureChannel2RGBA : public DataObject
{
public:
    virtual MappedTextureDataSet* createMappedDataSet() const;

    virtual void		setChannels(TextureChannels*);
    virtual void		notifyChannelChange()			{}
    virtual void		enableInterpolation(bool);
    virtual bool		interpolationEnabled() const;
    virtual bool		createRGBA(SbImagei32&) const		= 0;
				/*!<Fill the image with the output, using
				    current settings. */

    virtual bool		canSetSequence() const		{ return false;}
    virtual void		setSequence(int,const ColTab::Sequence&){}
    virtual const ColTab::Sequence* getSequence(int) const	{ return 0; }

    virtual void		swapChannels(int ch0,int ch1)	{}
    virtual void		setEnabled(int ch,bool yn)	{}
    virtual bool		isEnabled(int ch) const		{ return true; }

    virtual bool		canUseShading() const;
    virtual void		allowShading(bool);
    virtual bool		usesShading() const		= 0;
    virtual int			maxNrChannels() const		= 0;
    virtual int			minNrChannels() const		{ return 1; }

protected:
    			TextureChannel2RGBA();

    TextureChannels*	channels_;
    bool		shadingallowed_;
    bool		enableinterpolation_;

public:
    virtual void		notifyChannelInsert(int ch)	{}
    virtual void		notifyChannelRemove(int ch)	{}
};


/*!A destination where the texturechannels can put the mapped data. The class
   instanciation is provided by the TextureChannel2RGBA. */

mClass MappedTextureDataSet : public DataObject
{
public:
    virtual int		nrChannels() const			= 0;
    virtual bool	addChannel()				= 0;
    virtual bool	enableNotify(bool)			= 0;
    			//!<\returns previous status
    virtual void	touch()					= 0;
    virtual void	setNrChannels(int)			= 0;

    virtual void	setChannelData(int channel,const SbImagei32&) = 0;
			/*!<The SbImage's dataptr is assumed to
			    remain in memory until new SbImage comes,
			    or object is deleted. */
				 
    virtual const SbImagei32*	getChannelData() const			= 0;
};



/*! Implementation of TextureChannel2RGBA that takes a ColorSequence for each
channel and blends it into an RGBA image. */


mClass ColTabTextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static ColTabTextureChannel2RGBA*	create()
				mCreateDataObj(ColTabTextureChannel2RGBA);

    void			swapChannels(int ch0,int ch1);
    void			enableInterpolation(bool);

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

    bool			createRGBA(SbImagei32&) const;

    bool			canUseShading() const;
protected:
    void			notifyChannelChange()	{ update(); }
    void			adjustNrChannels() const;
    void			setChannels(TextureChannels*);

    					~ColTabTextureChannel2RGBA();

    void				update();
    void				getColors(int channel,
					      TypeSet<unsigned char>&) const;

    mutable ObjectSet<ColTab::Sequence>	coltabs_;
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
    SoComplexity*			shadingcomplexity_;

					//Non shading
    void				doFill(
	    				    SoColTabTextureChannel2RGBA*) const;
    SoGroup*				noneshadinggroup_;
    SoColTabTextureChannel2RGBA*	converter_;
    SoComplexity*			nonshadingcomplexity_;

    virtual SoNode*			gtInvntrNode();

    void				notifyChannelInsert(int ch);
    void				notifyChannelRemove(int ch);
};

} //namespace

#endif
