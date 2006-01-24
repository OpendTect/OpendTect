#ifndef vismultitexture_h
#define vismultitexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id: vismultitexture.h,v 1.3 2006-01-24 06:38:16 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

template <class T> class Array2D;

namespace visBase
{

class TextureInfo;
class VisColorTab;
class ColorSequence;

class MultiTexture : public DataObject
{
public:
    enum Operation	{ BLEND, ADD, REPLACE };
    enum Component	{ RED=1, GREEN=2, BLUE=4, OPACITY=8 };

    virtual bool	turnOn(bool yn)				= 0;
    virtual bool	isOn() const				= 0;
    virtual void	setTextureRenderQuality( float val )	= 0;
    			//!<\param val should be in the range between 0 and 1
    virtual float	getTextureRenderQuality() const		= 0;

    int			nrTextures() const;
    int			addTexture( const char* name );
    int			insertTexture( int, const char* name );
    void		removeTexture( int );
    void		swapTextures( int, int );
    void		setOperation( int texture, Operation  );
    Operation		getOperation( int texture ) const;
    void		setComponents( int texture, char bits );
    char		getComponents( int texture ) const;
    void		setColorTab( int texture, VisColorTab& );
    VisColorTab&	getColorTab( int texture );

    int			nrVersions( int texture ) const;
    void		setNrVersions( int texture, int nrvers );
    int			currentVersion( int texture ) const;
    void		setCurrentVersion( int texture, int version );

    const TypeSet<float>* getHistogram( int texture, int version ) const;

protected:
    			MultiTexture();
    bool		setTextureData( int texture, int version,
	    				const float*, int sz, bool managedata );
    bool		setTextureIndexData( int texture, int version,
					     const unsigned char*, int sz,
					     bool managedata );


    const unsigned char* getCurrentTextureIndexData( int texture ) const;

    virtual void	updateColorTables() {}
    virtual void	updateSoTextureInternal( int texture ) {}
    virtual void	insertTextureInternal( int texture ) {}
    virtual void	removeTextureInternal( int texture ) {}

    ObjectSet<TextureInfo>	textureinfo;

    friend		class TextureInfo;
    void		textureChange( TextureInfo* );
};


}; // Namespace

#endif
