#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.24 2003-01-24 11:31:13 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class AttribSliceSet;
class ColorTable;
class CubeSampling;

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
				mCreateDataObj(PlaneDataDisplay);

    void			setType(Type type);
    Type			getType() const { return type; }

    void			setOrigo( const Coord3& );
    void			setWidth( const Coord3& );
    				//!< Will only use the two coords that are valid

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed
    
    void			showDraggers(bool yn);
    void			resetManip();

    CubeSampling&		getCubeSampling(bool manippos=true);
    const CubeSampling&		getCubeSampling(bool manippos=true) const
				{ return const_cast<PlaneDataDisplay*>(this)->
				    	getCubeSampling(manippos); }
    void			setCubeSampling(const CubeSampling&);
    AttribSelSpec&		getAttribSelSpec();
    const AttribSelSpec&	getAttribSelSpec() const;
    void			setAttribSelSpec(AttribSelSpec&);
    bool			putNewData( AttribSliceSet* );
    				/*!< Becomes mine */
    const AttribSliceSet*	getPrevData() const;

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

    float			getValue( const Coord3& ) const;

    void			setMaterial( visBase::Material* );
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    const visBase::TextureRect&	textureRect() const { return *trect; }
    visBase::TextureRect&	textureRect() { return *trect; }

    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Coord3& ) const;
    Notifier<PlaneDataDisplay>	moving;

    const char*			getResName(int) const;
    void			setResolution(int);
    int				getResolution() const;
    int				getNrResolutions() const;

protected:
				~PlaneDataDisplay();
    void			appVelChCB(CallBacker*);
    void			manipChanged(CallBacker*);

    visBase::TextureRect*	trect;

    Type			type;
    AttribSelSpec&		as;
    AttribSliceSet*             cache;
    CubeSampling&		cs;
    CubeSampling&		manipcs;

    static const char*		trectstr;
};

};


#endif
