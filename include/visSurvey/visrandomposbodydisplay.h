#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"

namespace visBase { class RandomPos2Body; class Transformation; }
namespace EM { class RandomPosBody; }


namespace visSurvey
{
class Scene;

/*!\brief used for displaying a set of random picks in xyz coordinate.*/

mExpClass(visSurvey) RandomPosBodyDisplay : public visBase::VisualObjectImpl,
			      public visSurvey::SurveyObject
{ mODTextTranslationClass(RandomPosBodyDisplay);
public:
				RandomPosBodyDisplay();
				mDefaultFactoryInstantiation(
				 SurveyObject,RandomPosBodyDisplay,
				 "RandomPosBodyDisplay",
				 toUiString(sFactoryKeyword()));
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    DBKey			getDBKey() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    bool			setVisBody(visBase::RandomPos2Body*);
					//!<Creates an EM::Object for it.
    bool			setEMID(const DBKey&);
    DBKey			getEMID() const;
    EM::RandomPosBody*		getEMBody() const	{ return embody_; }

    const uiString&		errMsg() const { return errmsg_; }

protected:

    static const char*		sKeyPSEarthModelID()	{ return "EM ID"; }
    virtual			~RandomPosBodyDisplay();

    bool			updateVisFromEM();
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    const mVisTrans*		transform_;
    visBase::RandomPos2Body*	displaybody_;
    EM::RandomPosBody*		embody_;
};

};
