#ifndef viscallout_h
#define viscallout_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: viscallout.h,v 1.8 2007-09-14 07:41:33 cvsnanne Exp $
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

    void			setBoxColor(const Color&);
    const Color&		getBoxColor() const;

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
    void			urlClickCB(CallBacker*);
    void			setScaleTransform(visBase::DataObject*) const;

    visBase::Material*		boxmaterial_;
    visBase::Material*		textmaterial_;
    visBase::Material*		activedraggermaterial_;
};

} // namespace

#endif
