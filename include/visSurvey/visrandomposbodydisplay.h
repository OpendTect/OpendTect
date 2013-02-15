#ifndef visrandomposbodydisplay_h
#define visrandomposbodydisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"

namespace visBase { class RandomPos2Body; class Transformation; }
namespace EM { class RandomPosBody; }


namespace visSurvey
{
class Scene;

/*!
\brief Used for displaying a set of random picks in xyz coordinates.
*/

mExpClass(visSurvey) RandomPosBodyDisplay : public visBase::VisualObjectImpl,
       			      public visSurvey::SurveyObject
{
public:

    static RandomPosBodyDisplay* create()
				mCreateDataObj(RandomPosBodyDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setRightHandSystem(bool);

    bool			setVisBody(visBase::RandomPos2Body*);
    				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;
    EM::RandomPosBody*		getEMBody() const	{ return embody_; }

    const char*			errMsg() const { return errmsg_.str(); }
protected:

    static const char*		sKeyPSEarthModelID()	{ return "EM ID"; }
    virtual			~RandomPosBodyDisplay();
    
    void			updateVisFromEM();
    virtual void		fillPar(IOPar&,TypeSet<int>& saveids) const;
    virtual int			usePar(const IOPar&);

    const mVisTrans*		transform_;
    visBase::RandomPos2Body*	displaybody_;
    EM::RandomPosBody*		embody_;
};

};


#endif

