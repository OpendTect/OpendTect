#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.4 2002-04-22 14:41:18 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "callback.h"

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

class PlaneDataDisplay :  public visBase::VisualObject
{
public:

    enum Type			{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay*	create( Type type, visSurvey::Scene& scene_,
	    				const CallBack newdatacb )
				mCreateDataObj3arg( PlaneDataDisplay,
						Type, type,
						visSurvey::Scene&, scene_,
						const CallBack, newdatacb );

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed
    
    void			showDraggers(bool yn);
    void			resetManip();

    Type			getType() const { return type; }
    bool			getNewTextureData();
    CubeSampling&		getCubeSampling(bool manippos=true);
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

protected:
				~PlaneDataDisplay();

    void			select();
    void			deSelect();

    void			updateDraggerCB( CallBacker* = 0);

    visBase::TextureRect*	trect;

    Type			type;
    CubeSampling&		cs;
    AttribSelSpec&		as;
    CallBack			newdatacb;

    bool			selected_;
    bool			succeeded_;

    visSurvey::Scene&		scene;
};

};


#endif
