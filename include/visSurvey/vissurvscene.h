#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.25 2003-01-20 11:30:48 kristofer Exp $
________________________________________________________________________


-*/

#include "visscene.h"
#include "position.h"

namespace visBase
{
    class Annotation;
    class EventCatcher;
    class Transformation;
    class VisualObject;
};

namespace visSurvey
{

/*!
The display coordinate system is given in [m/m/ms] if the survey's depth is
given in time. If the survey's depth is given in meters, the display coordinate
system is given as [m/m/m]. The display coordinate system is _righthand_
oriented!

OpenInventor has hard to handle real-world coordinates (like xy UTM).
Therefore the coordinates given to OI must be transformed from the UTM system
to the display coordinate system. This is done by the display transform, which
is given to all objects in the UTM system. These object are responsible to
transform their coords themselves before giving them to OI.

The visSurvey::Scene has two domains:
1) the UTM coordinate system. It is adviced that most objects are here.
The objects added to this domain will have their transforms set to the
displaytransform which transforms their coords from UTM lefthand
(x, y, time[s] or depth[m] ) to display coords (righthand).

2) the InlCrl domain. Here, OI takes care of the transformation between
inl/crl/t to display coords, so the objects does not need any own transform.

*/

class Scene : public visBase::Scene
{
public:
    static Scene*		create()
				mCreateDataObj(Scene);

    virtual void		addObject( visBase::SceneObject* );
    virtual void		removeObject( int idx );

    void			showAnnotText(bool);
    bool			isAnnotTextShown() const;
    void			showAnnotScale(bool);
    bool			isAnnotScaleShown() const;
    void			showAnnot(bool);
    bool			isAnnotShown() const;

    Notifier<Scene>		mouseposchange;
    Coord3			getMousePos( bool xyt ) const;
    				/*! If ont xyt it is inlcrlt */
    float			getMousePosValue() const { return mouseposval;}

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    void			filterPicks( CallBacker* = 0 );
    void			updateRange();

protected:
    				~Scene();
    void			setCube();
    void			setup();

    virtual int			useOldPar( const IOPar& );

    void			addUTMObject( visBase::VisualObject* );
    void			addInlCrlTObject( visBase::VisualObject* );

    void			mouseMoveCB( CallBacker* = 0 );
    
    const visBase::Transformation*	zscaletransform;
    const visBase::Transformation*	inlcrl2displtransform;

    visBase::Annotation*		annot;
    visBase::EventCatcher*		eventcatcher;

    Coord3			xytmousepos;
    float			mouseposval;

    static const char*		annottxtstr;
    static const char*		annotscalestr;
    static const char*		annotcubestr;

				/* Only to be compatible with old par format */
    static const char*		displobjprefixstr;
    static const char*		nodisplobjstr;
    static const char*		xyzobjprefixstr;
    static const char*		noxyzobjstr;
    static const char*		xytobjprefixstr;
    static const char*		noxytobjstr;
    static const char*		inlcrltobjprefixstr;
    static const char*		noinlcrltobjstr;

};

};


#endif
