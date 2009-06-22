#ifndef vistopbotimage_h
#define	vistopbotimage_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		June 2009
 RCS:		$Id: vistopbotimage.h,v 1.1 2009-06-22 10:55:50 cvsranojay Exp $
________________________________________________________________________

-*/


#include "visobject.h"
#include "position.h"

namespace visBase
{

class FaceSet;
class Image;

mClass TopBotImage : public VisualObjectImpl
{
public:
    static TopBotImage*		create()
				mCreateDataObj(TopBotImage);

    void			setDisplayTransformation(mVisTrans*);
    void			setPos(const Coord& tl,const Coord& br,
				       float z);
    const Coord&		topLeft() const	    { return pos0_.coord(); }
    const Coord&		bottomRight() const { return pos2_.coord(); }

    void			setImageFilename(const char*);
    const char*			getImageFilename() const;

    void			setTransparency(int); // 0-255
    int				getTransparency() const; // returns value 0-255
    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    				~TopBotImage();

    void			updateCoords();

    FaceSet*			imgshape_;
    Image*			image_;
    Transformation*		trans_;
    Coord3			pos0_;
    Coord3			pos1_;
    Coord3			pos2_;
    Coord3			pos3_;
    BufferString		filenm_;
    
    static const char*		sKeyTopLeftCoord();
    static const char*		sKeyBottomRightCoord();
    static const char*		sKeyFileNameStr();
   
};

} // namespace visBase

#endif 
