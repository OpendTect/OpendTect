#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
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

mClass(Annotations) ImageDisplay : public visSurvey::LocationDisplay
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


mClass(Annotations) Image : public visBase::VisualObjectImpl
{
public:
    static Image*		create()
				mCreateDataObj(Image);
    void			setShape(visBase::FaceSet*);

    void			setPick(const Pick::Location&);
    void			setDisplayTransformation(const mVisTrans*);

protected:
    				~Image();

    const mVisTrans*		transform_;

    visBase::Transformation*	position_;
    visBase::ForegroundLifter*	lifter_;
    visBase::FaceSet*		shape_;
};



} // namespace

