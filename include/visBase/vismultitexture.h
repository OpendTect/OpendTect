#ifndef vismultitexture_h
#define vismultitexture_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"

template <class T> class Array2D;

namespace visBase
{

class TextureInfo;
class VisColorTab;
class ColorSequence;

mClass(visBase) MultiTexture : public DataObject
{
public:
    enum Operation		{ BLEND, ADD, REPLACE };
    enum Component		{ RED=1, GREEN=2, BLUE=4, OPACITY=8 };

    void			copySettingsFrom(int targettexture,
	    				const MultiTexture&,int srctexture);
    				/*!<Sets identical cctab, scaling, and flags.*/

    virtual bool		turnOn(bool yn)				= 0;
    virtual bool		isOn() const				= 0;
    virtual void		setTextureRenderQuality( float val )	= 0;
    				//!<\param val should be in the range between
				//!<0 and 1
    virtual float		getTextureRenderQuality() const		= 0;

    virtual int			maxNrTextures() const { return 0; }
    				/*!<\retval 0	Unlimited. */
    int				nrTextures() const;
    int				addTexture(const char* name);
    void			enableTexture(int texture,bool);
    bool			isTextureEnabled(int texture) const;
    void			setAngleFlag(int texture,bool);
    				/*!<Indicate that the values in the texture are
				    angles, i.e. -PI==PI */
    bool			isAngle(int texture) const;
    				/*!<Indicates that the values in the texture are
				    angles, i.e. -PI==PI */
    int				insertTexture(int,const char* name);
    void			removeTexture(int);
    virtual void		swapTextures(int,int);
    virtual void		setTextureTransparency(int,unsigned char)= 0;
    virtual unsigned char	getTextureTransparency(int) const 	= 0;
    bool			hasTransparency(int) const;
    virtual void		setOperation(int texture,Operation)	= 0;
    virtual Operation		getOperation(int texture) const		= 0;
    virtual void		setComponents(int texture,char bits);
    virtual char		getComponents(int texture) const;
    void			setColorTab(int texture,VisColorTab&);
    VisColorTab&		getColorTab(int texture);
    const VisColorTab&		getColorTab(int texture) const;

    int				nrVersions(int texture) const;
    void			setNrVersions(int texture,int nrvers);
    int				currentVersion(int texture) const;
    void			setCurrentVersion(int texture,int version);

    const TypeSet<float>*	getHistogram(int texture,int version) const;

protected:
    				MultiTexture();
    				~MultiTexture();
    bool			setTextureData(int texture,int version,
	    			       const float*,int sz,bool managedata);
    bool			setTextureIndexData(int texture,int version,
					    const unsigned char*,int sz,
					    bool managedata);

    const unsigned char*	getCurrentTextureIndexData(int texture) const;

    virtual void		updateColorTables() {}
    virtual void		updateSoTextureInternal(int texture)	{}
    virtual void		insertTextureInternal(int texture)	{}
    virtual void		removeTextureInternal(int texture)	{}

    ObjectSet<TextureInfo>	textureinfo_;

    friend			class TextureInfo;
    void			textureChange(TextureInfo*);
};


}; // Namespace

#endif

