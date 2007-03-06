#ifndef visannotimage_h
#define visannotimage_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: visannotimage.h,v 1.3 2007-03-06 07:56:00 cvskris Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase
{ class FaceSet; class Image; };

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

} // namespace

#endif
