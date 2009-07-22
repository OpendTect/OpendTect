#ifndef visannotimage_h
#define visannotimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: visannotimage.h,v 1.6 2009-07-22 16:01:26 cvsbert Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase
{
    class FaceSet;
    class Image;
    class ForegroundLifter;
}


namespace Annotations
{

/*!\brief
  Image
*/

class ImageDisplay : public visSurvey::LocationDisplay
{
public:
    static ImageDisplay*	create()
    				mCreateDataObj(ImageDisplay);

    bool			setFileName(const char*);
    const char*			getFileName() const;
    Notifier<ImageDisplay>	needFileName;

    void			setSet(Pick::Set*);
protected:
     visBase::VisualObject*	createLocation() const;
     void			setPosition(int, const Pick::Location&);
     void			dispChg(CallBacker* cb);

     bool			hasDirection() const { return false; }

    				~ImageDisplay();
    void			setScene(visSurvey::Scene*);
    void			updateCoords(CallBacker* = 0);
    int				isMarkerClick(const TypeSet<int>& path) const;

    visBase::FaceSet*		shape_;
    visBase::Image*		image_;
};


class Image : public visBase::VisualObjectImpl
{
public:
    static Image*		create()
				mCreateDataObj(Image);
    void			setShape(visBase::FaceSet*);

    void			setPick(const Pick::Location&);
    void			setDisplayTransformation(mVisTrans*);

protected:
    				~Image();

    visBase::Transformation*	transform_;

    visBase::Transformation*	position_;
    visBase::ForegroundLifter*	lifter_;
    visBase::FaceSet*		shape_;
};



} // namespace

#endif
