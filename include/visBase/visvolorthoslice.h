#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "position.h"
#include "visobject.h"

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
    static RefMan<OrthogonalSlice> create();
				mCreateDataObj(OrthogonalSlice);

    void			setVolumeDataSize(int xsz,int ysz,int zsz);
    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);
    void			setCenter(const Coord3& newcenter,bool alldims);

    int				getDim() const;
    void			setDim(int);

    void			getSliceInfo(int&,Interval<float>&) const;
    int				getSliceNr(int dim=-1) const;	// -1 : curdim_
    void			setSliceNr(int nr,int dim=-1);	// -1 : curdim_

    float			getPosition() const;

    const DepthTabPlaneDragger* getDragger() const;
    DepthTabPlaneDragger*	getDragger();
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

    RefMan<DepthTabPlaneDragger> dragger_;
    RefMan<TextureRectangle>	slice_;

    int				xdatasz_	= 0;
    int				ydatasz_	= 0;
    int				zdatasz_	= 0;
    int				curdim_		= 0;
    int				slicenr_[3];

    static const char*		dimstr();
    static const char*		slicestr();
};

} // namespace visBase
