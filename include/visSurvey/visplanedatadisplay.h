#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.10 2002-07-02 13:55:24 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"

class AttribSelSpec;
class CubeSampling;
class AttribSlice;

namespace Geometry { class Pos; }
namespace visBase { class TextureRect; };

namespace visSurvey
{

class Scene;

/*!\brief
    PlaneDataDisplay is a TextureRect that displays seismics or attributes.
*/

class PlaneDataDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    enum Type			{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay*	create()
				mCreateDataObj0arg(PlaneDataDisplay);

    void			setType(Type type);
    Type			getType() const { return type; }

    void			setOrigo( const Geometry::Pos& );
    void			setWidth( const Geometry::Pos& );
    				//!< Will only use the two coords that are valid

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed
    
    void			showDraggers(bool yn);
    void			resetManip();

    bool			updateAtNewPos();
    CubeSampling&		getCubeSampling(bool manippos=true);
    void			setCubeSampling(const CubeSampling&);
    AttribSelSpec&		getAttribSelSpec();
    void			setAttribSelSpec(AttribSelSpec&);
    bool			putNewData( AttribSlice* );
    void			operationSucceeded( bool yn=true )
				{ succeeded_ = yn; }

    void			turnOn(bool);
    bool			isOn() const;

    float			getValue( const Geometry::Pos& ) const;

    void			setMaterial( visBase::Material* );
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    const visBase::TextureRect&	textureRect() const { return *trect; }
    visBase::TextureRect&	textureRect() { return *trect; }

    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Geometry::Pos& ) const;
    virtual NotifierAccess*	getMovementNotification() { return &moving; }

protected:
				~PlaneDataDisplay();
    void			appVelChCB( CallBacker* );

    void			select();
    void			deSelect();

    visBase::TextureRect*	trect;

    Type			type;
    CubeSampling&		cs;
    AttribSelSpec&		as;

    bool			selected_;
    bool			succeeded_;

    Notifier<PlaneDataDisplay>	moving;

    static const char*		trectstr;
};

};


#endif
