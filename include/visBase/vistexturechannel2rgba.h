#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2008
________________________________________________________________________


-*/

#include "visdata.h"


namespace ColTab { class Sequence; }
namespace osgGeo { class LayeredTexture; class ColorSequence; }
namespace osg { class Image; }

namespace visBase
{

class TextureChannels;
class MappedTextureDataSet;

/*!
Interface for classes that can take one or more texture channels in an
TextureChannels class and convert them to RGBA textures in OpenGL, optionally
with shaders. There should always be a non-shading way to fall back on.
*/

mExpClass(visBase) TextureChannel2RGBA : public DataObject
{ mODTextTranslationClass(TextureChannel2RGBA);
public:
    virtual MappedTextureDataSet* createMappedDataSet() const;

    virtual void		setChannels(TextureChannels*);
    virtual void		notifyChannelChange()			{}
    virtual void		notifyChannelInsert(int ch)		{}
    virtual void		notifyChannelRemove(int ch)		{}

    virtual const osg::Image*	createRGBA() const;
				/*!<Fill the image with the output, using
				    current settings. */

    virtual bool		canSetSequence() const		{ return false;}
    virtual void		setSequence(int,const ColTab::Sequence&){}
    virtual const ColTab::Sequence& getSequence(int) const	= 0;

    virtual void		swapChannels(int ch0,int ch1)	{}
    virtual void		setEnabled(int ch,bool yn)	{}
    virtual bool		isEnabled(int ch) const		{ return true; }

    virtual bool		canUseShading() const;
    virtual void		allowShading(bool);
    virtual bool		usesShading() const;

    virtual int			maxNrChannels() const		= 0;
    virtual int			minNrChannels() const		{ return 1; }
    virtual void		getChannelName(int,uiString&) const;

    int				getTexturePixelSizeInBits() const;
    const unsigned char*	getTextureData() const;
    int				getTextureWidth() const;
    int				getTextureHeight() const;

protected:
				TextureChannel2RGBA();

    TextureChannels*		channels_;
    osgGeo::LayeredTexture*     laytex_;
};


/*!A destination where the texturechannels can put the mapped data. The class
   instantiation is provided by the TextureChannel2RGBA. */

mExpClass(visBase) MappedTextureDataSet : public DataObject
{
public:
    virtual int		nrChannels() const			= 0;
    virtual bool	addChannel()				= 0;
    virtual bool	enableNotify(bool)			= 0;
			//!<\returns previous status
    virtual void	touch()					= 0;
    virtual void	setNrChannels(int)			= 0;
};



/*! Implementation of TextureChannel2RGBA that takes a ColorSequence for each
channel and blends it into an RGBA image. */


mExpClass(visBase) ColTabTextureChannel2RGBA : public TextureChannel2RGBA
{
public:

    typedef ColTab::Sequence		Sequence;

    static ColTabTextureChannel2RGBA*	create()
			mCreateDataObj(ColTabTextureChannel2RGBA);

    void		swapChannels(int ch0,int ch1);

    bool		canSetSequence() const		{ return true;}
    void		setSequence(int ch,const Sequence&);
    const Sequence&	getSequence(int ch) const;

    void		setEnabled(int ch,bool yn);
    bool		isEnabled(int ch) const;

    void		setTransparency(int ch,unsigned char yn);
    unsigned char	getTransparency(int ch) const;

    int			maxNrChannels() const;

protected:

    void		notifyChannelInsert(int ch);
    void		notifyChannelRemove(int ch);

    void		adjustNrChannels();
    void		setChannels(TextureChannels*);

			~ColTabTextureChannel2RGBA();

    void		update();
    void		updFromSeq(int);
    void		getColors(int channel,TypeSet<unsigned char>&) const;
    void		colSeqModifCB(CallBacker*);
    void		startMonitorSeq(int);
    void		stopMonitorSeq(int);

    TypeSet<ConstRefMan<Sequence> >	colseqs_;
    BoolTypeSet				enabled_;
    TypeSet<unsigned char>		opacity_;

    ObjectSet<osgGeo::ColorSequence>	osgcolsequences_;
    ObjectSet<TypeSet<unsigned char> >	osgcolseqarrays_;

};

} //namespace
