#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.12 2003-01-16 15:33:01 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class CubeSampling;
class AttribSliceSet;

namespace visBase
{
    class CubeView;
    class VisColorTab;
};

namespace visSurvey
{

class Scene;

/*!\brief
    VolumeDisplay is a 3DTexture with 3 planes that display seismics or attributes.
*/

class VolumeDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    static VolumeDisplay*	create()
				mCreateDataObj(VolumeDisplay);

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
    const AttribSelSpec&	getAttribSelSpec() const;
    void			setAttribSelSpec(AttribSelSpec&);
    CubeSampling&		getCubeSampling();
    const CubeSampling&		getCubeSampling() const
				{ return const_cast<VolumeDisplay*>(this)->
				    	getCubeSampling(); }
    void			setCubeSampling(const CubeSampling&);
    bool			putNewData( AttribSliceSet* );
    const AttribSliceSet*	getPrevData() const;
    void			operationSucceeded( bool yn=true )
				{ succeeded_ = yn; }

    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTable(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTable();
    void			setClipRate(float);
    float			clipRate() const;
    void			setAutoscale(bool);
    bool			autoScale() const;

    void                        setMaterial( visBase::Material* );
    				/*!< Does not affect the volren */
    visBase::Material*		getMaterial();
    				/*!< Does not affect the volren */
    const visBase::Material*	getMaterial() const;
    				/*!< Does not affect the volren */

    void			showVolRen( bool );
    bool			isVolRenShown() const;

    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    Notifier<VolumeDisplay>	moved;
    Notifier<VolumeDisplay>	slicemoving;


protected:
				~VolumeDisplay();

    void			select();
    void			deSelect();
    void			manipFinished(CallBacker*);
    void			manipInMotion(CallBacker*);
    void			initSlice(int);
    void			sliceMoving(CallBacker*);

    visBase::CubeView*		cube;
    AttribSliceSet*		cache;

    AttribSelSpec&		as;

    bool			selected_;
    bool			succeeded_;
    bool			manipulated;

    int				inlid;
    int				crlid;
    int				tslid;

    static const char*		volumestr;
};

};


#endif
