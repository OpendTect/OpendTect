#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.5 2002-10-23 09:42:07 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class CubeSampling;
class AttribSliceSet;
class ColorTable;

namespace visBase { class CubeView; };

namespace visSurvey
{

class Scene;

/*!\brief
    VolumeDisplay is a 3DTexture with 3 rectangles that display seismics or attributes.
*/

class VolumeDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    static VolumeDisplay*	create()
				mCreateDataObj0arg(VolumeDisplay);

    void			setCenter( const Coord3& );
    Coord3			center() const;
    void			setWidth( const Coord3& );
    Coord3			width() const;

    void			showBox(bool yn);
    void			resetManip();
    void			getPlaneIds(int&,int&,int&);
    float			getPlanePos(int);

    bool			updateAtNewPos();
    AttribSelSpec&		getAttribSelSpec();
    void			setAttribSelSpec(AttribSelSpec&);
    CubeSampling&		getCubeSampling();
    CubeSampling&		getPrevCubeSampling()	{ return prevcs; }
    void			setCubeSampling(const CubeSampling&);
    bool			putNewData( AttribSliceSet* );
    AttribSliceSet*		getPrevData();
    void			operationSucceeded( bool yn=true )
				{ succeeded_ = yn; }

    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTable(const ColorTable&);
    const ColorTable&		getColorTable() const;
    void			setClipRate(float);
    float			clipRate() const;
    void			setAutoscale(bool);
    bool			autoScale() const;
    void			setDataRange(const Interval<float>&);
    Interval<float>		getDataRange() const;

    void                        setMaterial( visBase::Material* );
    const visBase::Material*    getMaterial() const;
    visBase::Material*          getMaterial();

    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual NotifierAccess*	getMovementNotification() { return &moving; }
    Notifier<VolumeDisplay>	rectmoving;


protected:
				~VolumeDisplay();

    void			select();
    void			deSelect();
    void			manipFinished(CallBacker*);
    void			manipInMotion(CallBacker*);
    void			rectInMotion(CallBacker*);

    visBase::CubeView*		cube;

    AttribSelSpec&		as;
    CubeSampling&		prevcs;

    bool			selected_;
    bool			succeeded_;
    bool			manipulated;

    Notifier<VolumeDisplay>	moving;

    static const char*		volumestr;
};

};


#endif
