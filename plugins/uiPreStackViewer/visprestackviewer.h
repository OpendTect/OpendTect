#ifndef visprestackviewer_h
#define visprestackviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2007
 RCS:		$Id: visprestackviewer.h,v 1.3 2007-09-20 21:27:00 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstring.h"
#include "position.h"
#include "vissurvobj.h"
#include "visobject.h"

class Coord3;
template <class T> class Interval;

namespace visBase 
{
    class Coordinates;                
    class DepthTabPlaneDragger;
    class FaceSet;
    class FlatViewer;
    class PickStyle;
    class Scene;
};

namespace visSurvey 
{
    class PlaneDataDisplay;
    class MultiTextureSurveyObject;
};

namespace PreStackView
{

class PreStackViewer :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static PreStackViewer*	create()
				mCreateDataObj( PreStackViewer );
    void			allowShading(bool yn);
    void			setColor(Color);			
    void			setMultiID(const MultiID& mid);

    bool			isInlCrl() const { return true; }

    void			setPosition(const BinID&);
    const BinID&		getPosition() const;
    void			setSectionDisplay(visSurvey::PlaneDataDisplay*);
    const visSurvey::PlaneDataDisplay* getSectionDisplay() const;		

    bool                        displayAutoWidth() const { return autowidth_; }
    void                        displaysAutoWidth(bool yn);
    bool                        displayOnPositiveSide() const 
    							{return positiveside_; }
    void                        displaysOnPositiveSide(bool yn);
    float                       getFactor() { return factor_; }
    void			setFactor(float scale);
    float                       getWidth() { return width_; }
    void			setWidth(float width);
    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
				  		BufferString& val,
						BufferString& info) const;
    void			 otherObjectsMoved( 
	    				const ObjectSet<const SurveyObject>&, 
					int whichobj );
protected:
    					~PreStackViewer();
    void				dataChangedCB(CallBacker*);
    void				setDisplayTransformation(mVisTrans*);
    void				sectionMovedCB(CallBacker*);

    void				draggerMotion(CallBacker*);
    void				finishedCB(CallBacker*);
    void				mouseMoveCB(CallBacker*);

    BinID				bid_;
    visBase::DepthTabPlaneDragger* 	planedragger_;
    visBase::FaceSet*                   draggerrect_;
    visBase::FlatViewer*		flatviewer_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			pickstyle_;
    
    MultiID				mid_;
    visSurvey::PlaneDataDisplay*	section_;

    bool				positiveside_;
    bool				autowidth_;
    float				factor_;
    float				width_;
};

}; //namespace

#endif
