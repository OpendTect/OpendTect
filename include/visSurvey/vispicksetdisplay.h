#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.31 2004-04-29 13:03:06 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"

class Color;
class IOPar;
class Sphere;
class PickSet;

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

    void			copyFromPickSet( const PickSet& );
    void			copyToPickSet( PickSet& ) const;

    void			addPick(const Coord3&,const Sphere&);
    void			addPick(const Coord3&);
    int				nrPicks() const;
    Coord3			getPick(int idx) const;
    Coord3			getDirection(int idx) const;
    void			removePick(const Coord3&);
    void			removeAll();

    float			getInitSize() const	{ return initsz; }
    float			getPickSize() const 	{ return picksz; }
    void			setSize(float);

    void			setColor(const Color&);
    const Color&		getColor() const;

    void			setType(int);
    int				getType() const;
    void			getTypeNames(TypeSet<char*>&);

    void			showAll(bool yn);
    bool			allShown() const	{ return showall; }
    void			filterPicks( ObjectSet<SurveyObject>&,
	    				     float dist );

    virtual void                fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int                 usePar( const IOPar& );

    Notifier<PickSetDisplay>	changed;

    void			setTransformation( visBase::Transformation* );
    visBase::Transformation*	getTransformation();

protected:
    virtual			~PickSetDisplay();

    void			pickCB( CallBacker* =0 );
    void			updatePickSz( CallBacker* = 0);

    int				picktype;
    float			picksz;
    const float			initsz;

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
