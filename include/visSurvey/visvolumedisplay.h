#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.1 2002-08-20 07:39:28 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"

class AttribSelSpec;
class CubeSampling;
class AttribSlice;

namespace Geometry { class Pos; }
namespace visBase { class CubeView; };

namespace visSurvey
{

class Scene;

/*!\brief
    VolumeDisplay is a TextureRect that displays seismics or attributes.
*/

class VolumeDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    static VolumeDisplay*	create()
				mCreateDataObj0arg(VolumeDisplay);

    void			setCenter( const Geometry::Pos& );
    Geometry::Pos		center() const;
    void			setWidth( const Geometry::Pos& );
    Geometry::Pos		width() const;

    void			showDraggers(bool yn);
    void			resetManip();
    void			getPlaneIds(int&,int&,int&);
    float			getPlanePos(int);

    bool			updateAtNewPos();
    CubeSampling&		getCubeSampling();
    void			setCubeSampling(const CubeSampling&);
    AttribSelSpec&		getAttribSelSpec();
    void			setAttribSelSpec(AttribSelSpec&);
    bool			putNewData( AttribSlice* );
    void			operationSucceeded( bool yn=true )
				{ succeeded_ = yn; }

    void			turnOn(bool);
    bool			isOn() const;

    void                        setMaterial( visBase::Material* ) {}
    const visBase::Material*    getMaterial() const 		{ return 0; }
    visBase::Material*          getMaterial() 			{ return 0; }

    SoNode*			getData();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );


protected:
				~VolumeDisplay();

    void			select();
    void			deSelect();
    void			manipFinished(CallBacker*);
    void			manipInMotion(CallBacker*);

    visBase::CubeView*		cube;

    AttribSelSpec&		as;

    bool			selected_;
    bool			succeeded_;

    static const char*		volumestr;
};

};


#endif
