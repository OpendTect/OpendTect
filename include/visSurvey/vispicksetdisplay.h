#ifndef vispicksetdisplay_h
#define vispicksetdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.50 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "bufstringset.h"

class Color;
class IOPar;
class Sphere;
namespace Pick { class Set; class Location; }

namespace visBase
{
    class DataObjectGroup;
    class EventCatcher;
    class Transformation;
};


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying picks.

  Picks are positions in 3d (x,y,z) selected by the user by mouseclicks,
  or generated at random. Picks have a constant size in pixels, and can be
  visualized by a number of shapes.
*/

class PickSetDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static PickSetDisplay*	create()
				mCreateDataObj(PickSetDisplay);
    void			setSet(Pick::Set*); // once!
    Pick::Set*			getSet()		{ return set_; }

    void			fullRedraw();
    void			showAll(bool yn);
    bool			allShown() const	{ return showall; }

    virtual BufferString	getManipulationString() const;
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						const Coord3&,float&,
						BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual Color		getColor() const;
    virtual bool		isPicking() const;
    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    virtual NotifierAccess*	getManipulationNotifier() { return &visnotif_; }
    virtual void		setDisplayTransformation(
	    					visBase::Transformation*);
    virtual visBase::Transformation* getDisplayTransformation();
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:

    Pick::Set*			set_;
    Notifier<PickSetDisplay>	visnotif_;

    virtual			~PickSetDisplay();

    Pick::Location&		addPick(const Coord3&,const Sphere&,bool);
    void			addDisplayPick(const Pick::Location&);

    void			pickCB(CallBacker* cb=0);
    void			locChg(CallBacker* cb=0);
    void			setChg(CallBacker* cb=0);
    void			dispChg(CallBacker* cb=0);

    bool			showall;
    int				mousepressid;
    Coord3		        mousepressposition;

    visBase::DataObjectGroup*	group;
    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;

    static const char*		nopickstr;
    static const char*		pickprefixstr;
    static const char*		showallstr;
    static const char*		shapestr;
    static const char*		sizestr;
};

};


#endif
