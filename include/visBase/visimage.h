#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdata.h"
#include "odimage.h"

template <class T> class Array2D;

class SoTexture2;
class SbImage;


namespace visBase
{

/*!Displays an image that either is read from disk or in memory. */

mExpClass(visBase) Image : public DataObject
{
public:
    static Image*	create()
			mCreateDataObj( Image );

    bool		replacesMaterial() const;
    void		replaceMaterial(bool yn);

    void		setData(const Array2D<Color>&,bool trans);
    void		setFileName(const char*);
    const char*		getFileName() const;

protected:
    			~Image();
    SoTexture2*		texture_;

    virtual SoNode*	gtInvntrNode();

};


mExpClass(visBase) RGBImage : public OD::RGBImage
{
public:
    		RGBImage();
    		RGBImage(SbImage*);

    bool	hasAlpha() const;
    char	nrComponents() const;
    bool	setSize(int,int);
    int		getSize(bool xdir) const;
    Color	get(int,int) const;
    bool	set(int,int,const Color&);

    void	fill(unsigned char*) const;
protected:

    SbImage*	image_;
};

} // namespace visBase
