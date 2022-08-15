#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"


namespace visBase
{
class DepthTabPlaneDragger;
class TextureRectangle;
class TextureChannels;

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
    int			getSliceNr(int dim=-1) const;	// -1 : curdim_
    void			setSliceNr(int nr,int dim=-1);	// -1 : curdim_

    float			getPosition() const;

    DepthTabPlaneDragger*	getDragger() const;
    void			enablePicking(bool);
    bool			isPickingEnabled() const;
    void			removeDragger();

    NotifierAccess&		dragStart();
    Notifier<OrthogonalSlice>	motion;
    NotifierAccess&		dragFinished();

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setTextureChannels(TextureChannels*);
    TextureChannels*		getTextureChannels();

protected:
				~OrthogonalSlice();

    void			draggerMovementCB(CallBacker*);

    DepthTabPlaneDragger*	dragger_;
    TextureRectangle*		slice_;

    int				xdatasz_, ydatasz_, zdatasz_;
    int			curdim_;
    int			slicenr_[3];

    static const char*		dimstr();
    static const char*		slicestr();
};

} // namespace visBase
