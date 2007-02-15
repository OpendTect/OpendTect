#ifndef viscallout_h
#define viscallout_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: viscallout.h,v 1.5 2007-02-15 20:34:12 cvskris Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace Annotations
{

/*!\brief
  Callout
*/

class CalloutDisplay : public visSurvey::LocationDisplay
{
public:
    static CalloutDisplay*	create()
    				mCreateDataObj(CalloutDisplay);

    void			setScale(float);
    float			getScale() const;

    static const char*		sKeyText() 		{ return "T"; }
    static const char*		sKeyURL() 		{ return "U"; }

protected:
     visBase::VisualObject*	createLocation() const;

     void			showManipulator(bool);
     bool			isManipulatorShown() const;
     void			setPosition(int loc,const Pick::Location& );
     bool			hasDirection() const { return true; }
     bool			hasText() const { return true; }
     int			isMarkerClick(const TypeSet<int>&) const;

    				~CalloutDisplay();
    float			scale_;
    void			setScene(visSurvey::Scene*);
    void			zScaleChangeCB(CallBacker*);
    void			directionChangeCB(CallBacker*);
    void			setScaleTransform(visBase::DataObject*) const;

    visBase::Material*		markermaterial_;
    visBase::Material*		boxmaterial_;
    visBase::Material*		textmaterial_;
};

} // namespace

#endif
