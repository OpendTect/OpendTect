#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.14 2002-10-09 11:29:32 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class AttribSlice;
class ColorTable;
class CubeSampling;

namespace Geometry { class Pos; }
namespace visBase { class TextureRect; class VisColorTab; };

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
    CubeSampling&		getPrevCubeSampling()	{ return prevcs; }
    CubeSampling&		getCubeSampling(bool manippos=true);
    void			setCubeSampling(const CubeSampling&);
    AttribSelSpec&		getAttribSelSpec();
    void			setAttribSelSpec(AttribSelSpec&);
    bool			putNewData( AttribSlice* );
    AttribSlice*		getPrevData();
    void			operationSucceeded( bool yn=true )
				{ succeeded_ = yn; }

    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTable(const ColorTable&);
    void			setColorTable(visBase::VisColorTab*);
    const ColorTable&		getColorTable() const;
    void			setClipRate(float);
    float			clipRate() const;
    void			setAutoscale(bool);
    bool			autoScale() const;
    void			setDataRange(const Interval<float>&);
    Interval<float>		getDataRange() const;

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

    const char*			getResName(int);
    void			setResolution(int);
    int				getResolution() const;
    int				getNrResolutions() const;

protected:
				~PlaneDataDisplay();
    void			appVelChCB( CallBacker* );

    void			select();
    void			deSelect();

    visBase::TextureRect*	trect;

    Type			type;
    CubeSampling&		cs;
    CubeSampling&		prevcs;
    AttribSelSpec&		as;

    bool			selected_;
    bool			succeeded_;

    Notifier<PlaneDataDisplay>	moving;

    static const char*		trectstr;
};

};


#endif
