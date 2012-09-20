#ifndef viscallout_h
#define viscallout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vislocationdisplay.h"


namespace visBase
{
    class Rotation;
    class Anchor;
    class TextBox;
    class Dragger;
    class FaceSet;
    class RotationDragger;
    class TriangleStripSet;
};

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

    void			setDisplayTransformation(const mVisTrans*);

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


class Callout : public visBase::VisualObjectImpl
{
public:
    static Callout*		create()
				mCreateDataObj(Callout);

    Sphere			getDirection() const;
    Notifier<Callout>		moved;
    NotifierAccess&		urlClick();
    const CallBacker*		getAnchor() const;

    void			setPick(const Pick::Location&);
    void			setTextSize(float ns);
    void			setFeedbackMaterial(visBase::Material*);
    void			setActiveFeedbackMaterial(visBase::Material*);
    void			setMarkerMaterial(visBase::Material*);
    void			setBoxMaterial(visBase::Material*);
    void			setTextMaterial(visBase::Material*);
    void			setZScale(float s)	{ zscale_ = s; }
    void			setScale(visBase::Transformation*);
    visBase::Transformation*	getScale() { return scale_; }
    void			reportChangedScale() { updateArrow(); }

    void			displayMarker(bool);
    				//!<Aslo controls the draggers. */
    bool			isMarkerDisplayed() const;
    void			setDisplayTransformation(const mVisTrans*);
    int				getMarkerID() const;

protected:
    				~Callout();
    void			updateCoords();
    void			updateArrow();
    void			setText(const char*);
    void			dragStart(CallBacker*);
    void			dragChanged(CallBacker*);
    void			dragStop(CallBacker*);
    void			setupRotFeedback();

    const mVisTrans*		displaytrans_;

    visBase::Marker*		marker_; //In normal space

    visBase::Transformation*	object2display_; //Trans to object space
    visBase::Rotation*		rotation_; 
    visBase::Transformation*	scale_;	 

    visBase::PolygonOffset*	calloutoffset_;	//In object space
    visBase::Anchor*		anchor_;
    visBase::TextBox*		fronttext_;
    visBase::FaceSet*		faceset_;	
    visBase::Dragger*		translationdragger_;	

    visBase::RotationDragger*	rotdragger_;	
    visBase::TriangleStripSet*	rotfeedback_;	
    visBase::TriangleStripSet*	rotfeedbackactive_;

    float			rotfeedbackradius_;
    Coord3			rotfeedbackpos_;
    Coord3			dragstarttextpos_;
    Coord3			dragstartdraggerpos_;

    visBase::Rotation*		backtextrotation_; //In backtext space
    visBase::TextBox*		backtext_;

    bool			isdragging_;
    float			zscale_;

};



} // namespace

#endif
