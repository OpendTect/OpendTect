#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.27 2003-11-07 12:21:55 bert Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class ColorAttribSel;
class CubeSampling;
class AttribSliceSet;

namespace visBase
{
    class CubeView;
    class VisColorTab;
    class Material;
};

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying 3D volumes
  
  VolumeDisplay is the front-end object for displaying 3D volumes and Volume
  Rendering. It has a <code>visBase::CubeView>/code> object which has a 
  3DTexture with an inline-, crossline-, and timeslice plane for displaying the
  data.
*/

class VolumeDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    static VolumeDisplay*	create()
				mCreateDataObj(VolumeDisplay);

    void			setCenter( const Coord3& );
    Coord3			center() const;
    Coord3			manipCenter() const;
    void			setWidth( const Coord3& );
    Coord3			width() const;
    Coord3			manipWidth() const;

    void			showBox(bool yn);
    void			resetManip();
    void			getPlaneIds(int&,int&,int&);
    float			getPlanePos(int);

    bool			updateAtNewPos();
    AttribSelSpec&		getAttribSelSpec();
    const AttribSelSpec&	getAttribSelSpec() const;
    void			setAttribSelSpec(const AttribSelSpec&);
    CubeSampling&		getCubeSampling(bool manippos=true);
    const CubeSampling&		getCubeSampling(bool manippos=true) const
				{ return const_cast<VolumeDisplay*>(this)->
				    	getCubeSampling(manippos); }
    void			setCubeSampling(const CubeSampling&);
    bool			putNewData(AttribSliceSet*,bool);
    const AttribSliceSet*	getPrevData() const;
    float			getValue(const Coord3&) const;

    ColorAttribSel&		getColorSelSpec();
    const ColorAttribSel&	getColorSelSpec() const;
    void			setColorSelSpec(const ColorAttribSel&);

    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTab(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTab();
    const visBase::VisColorTab&	getColorTab() const;
    const TypeSet<float>& 	getHistogram() const;

    void                        setMaterial( visBase::Material* );
    				/*!< Does not affect the volren */
    visBase::Material*		getMaterial();
    				/*!< Does not affect the volren */
    const visBase::Material*	getMaterial() const;
    				/*!< Does not affect the volren */

    int				getVolRenId() const;
    bool			isVolRenShown() const;
    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    Notifier<VolumeDisplay>	slicemoving;


protected:
				~VolumeDisplay();
    void			setCube(visBase::CubeView*);

    void			initSlice(int);
    void			manipMotionFinishCB(CallBacker*);
    void			sliceMoving(CallBacker*);
    void			setData(const AttribSliceSet*,int datatype=0);

    visBase::CubeView*		cube;
    AttribSliceSet*		cache;
    AttribSliceSet*		colcache;

    AttribSelSpec&		as;
    ColorAttribSel&		colas;

    int				inlid;
    int				crlid;
    int				tslid;

    static const char*		volumestr;
    static const char*		inlinestr;
    static const char*		crosslinestr;
    static const char*		timestr;
    static const char*		volrenstr;
    static const char*		inlineposstr;
    static const char*		crosslineposstr;
    static const char*		timeposstr;
    static const char*		inlineshowstr;
    static const char* 		crosslineshowstr;
    static const char* 		timeshowstr;

};

};


#endif
