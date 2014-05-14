#ifndef visimagerect_h
#define visimagerect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"

namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }

namespace visBase
{

/*!Displays an image that either is read from disk or in memory. */

mExpClass(visBase) ImageRect : public VisualObjectImpl
{
public:
    static ImageRect*	create()
			mCreateDataObj( ImageRect );
    
    void		setCenterPos(const Coord3&);
    void		setCornerPos(const Coord3& tl,const Coord3& br);

    void		setFileName(const char*);
    const char*		getFileName() const;

    struct ImageData
    {
	BufferString	    data_;
	int		    width_;
	int		    height_;
	int		    depth_;
	int		    internalformat_;
	int		    format_;
	int		    datatype_;
	int		    packing_;
    };


    void		setImageData(const ImageData&);
    ImageData		getImageData() const;

    void		setDisplayTransformation(const mVisTrans*);
  
protected:
    			    ~ImageRect();

    const mVisTrans*		trans_;
    int				layerid_;
    osgGeo::LayeredTexture*	laytex_;
    osgGeo::TexturePlaneNode*	texplane_;
    ImageData			imagedata_;
    BufferString		fnm_;
};


}; // namespace visBase

#endif

