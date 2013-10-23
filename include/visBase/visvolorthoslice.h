#ifndef visvolorthoslice_h
#define visvolorthoslice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"


namespace visBase
{
class DepthTabPlaneDragger;

/*!\brief
Slice that cuts orthogonal through a VolumeData.
*/

mExpClass(visBase) OrthogonalSlice : public visBase::VisualObjectImpl
{
public:
    static OrthogonalSlice*	create()
				mCreateDataObj(OrthogonalSlice);

    void			setVolumeDataSize(int xsz,int ysz,int zsz);
    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);
    void			setCenter(const Coord3& newcenter,bool alldims);

    int				getDim() const;
    void			setDim(int);

    void			getSliceInfo(int&,Interval<float>&) const;
    int				getSliceNr() const;
    void			setSliceNr( int );

    float			getPosition() const;

    visBase::DepthTabPlaneDragger* getDragger() const;

    void			removeDragger();

    NotifierAccess&		dragStart();
    Notifier<OrthogonalSlice>	motion;
    NotifierAccess&		dragFinished();

protected:
				~OrthogonalSlice();

    void			draggerMovementCB(CallBacker*);
    
    visBase::DepthTabPlaneDragger* dragger_;
/* OSG-TODO: Replace with  OSG equivalent if class is prolongated
    SoOrthoSlice*		slice_;
*/
    int				xdatasz_, ydatasz_, zdatasz_;

    static const char*		dimstr();
    static const char*		slicestr();
};


};
	
#endif

