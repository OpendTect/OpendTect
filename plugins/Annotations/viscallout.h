#ifndef viscallout_h
#define viscallout_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: viscallout.h,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________


-*/

//#include "vistext.h"
#include "vislocationdisplay.h"
//#include "position.h"

namespace visSurvey
{

/*!\brief
  Callout
*/

class Coordinates;
class Cube;
class EventCatcher;
class FaceSet;
class IndexedPolyLine;

class CalloutDisplay : public LocationDisplay
{
public:
    static CalloutDisplay*	create()
    				mCreateDataObj(CalloutDisplay);

    void			setScale(const Coord3&);
    Coord3			getScale() const;

protected:
     visBase::VisualObject*	createLocation() const;
     void			setPosition(int loc,const Pick::Location& );
     bool			hasDirection() const { return true; }

    				~CalloutDisplay();
    Coord3			scale_;
    visBase::Material*		markermaterial_;
    visBase::Material*		boxmaterial_;
    visBase::Material*		textmaterial_;

    /*
    void			createFaceNode();
    void			createMarkerNode();
    void			createLineNode();
    void			createTextNode();

    SoAsciiText*		text_;
    SoRotationXYZ*		zrotation_;
    SoScale*			scale_;

    FaceSet*			faceset_;
    IndexedPolyLine*		border_;
    Cube*			marker_;
    EventCatcher*		eventcatcher_;

    int				orientation_;
    bool			movemarker_;
    bool			moveface_;

    void			updateBackground();
    void			setBackgroundCoords(const float*,const float*);
    void			setBackgroundIndices();
    void			setArrowCoord();
    void			updateArrowIndices(int);

    void			mouseCB(CallBacker*);
    */
};

} // namespace visBase

#endif
