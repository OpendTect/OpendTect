#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "volprocchain.h"
#include "enums.h"

namespace VolProc
{

mExpClass(VolumeProcessing) VoxelConnectivityFilter : public Step
{ mODTextTranslationClass(VoxelConnectivityFilter);
public:

			mDefaultFactoryInstantiation( Step,
				VoxelConnectivityFilter,
				"VoxelConnectivityFilter",
				tr("Voxel Connectivity Filter") );

    enum		Connectivity { Faces, Edges, Corners };
			mDeclareEnumUtils( Connectivity );

    enum		AcceptOutput { Ranking, BodySize, Value, Transparent };
			mDeclareEnumUtils( AcceptOutput );

    void		setConnectivity(Connectivity val){connectivity_ = val;}
			/*!<Must be either 6, 12 or 26 */
    Connectivity	getConnectivity() const		{ return connectivity_;}

    void		setMinimumBodySize(od_int64 val){ minbodysize_ = val; }
    od_int64		getMinimumBodySize() const	{ return minbodysize_; }

    void		setAcceptRange(const Interval<float>& v) { range_ = v; }
			/*!<If start is udf, kept range is less than stop.
                          If stop is udf, kept range is more than start.
                          If range is reversed, outside range is kept.
                          Otherwise, inside range is kept. */

    const Interval<float>& getAcceptRange() const { return range_; }

    void		setRejectValue(float v)  { rejectvalue_ = v; }
    float		getRejectValue() const { return rejectvalue_; }

    void		setAcceptOutput(AcceptOutput v) {acceptoutput_=v;}
    AcceptOutput	getAcceptOutput() const { return acceptoutput_; }

    void		setAcceptValue(float val) { acceptvalue_ = val; }
    float		getAcceptValue() const  { return acceptvalue_; }


    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    bool		needsInput() const override		{ return true; }
    bool		canInputAndOutputBeSame() const override{ return true; }
    bool		needsFullVolume() const override	{ return true; }
    bool		areSamplesIndependent() const override { return false; }
    uiString		errMsg() const override		{ return errmsg_; }

    Task*		createTask() override;

    mDeprecatedDef od_int64	getProcTimeExtraMemory() const override
				{ return 0; }

    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

			VoxelConnectivityFilter();
			~VoxelConnectivityFilter();

    static const char*	sKeyRange()		{ return "Range"; }
    static const char*	sKeyConnectivity()	{ return "Connectivity"; }
    static const char*	sKeyMinimumSize()	{ return "Minimum size"; }
    static const char*	sKeyRejectValue()	{ return "Rejected Value"; }
    static const char*	sKeyAcceptOutput()	{ return "Accepted Output"; }
    static const char*	sKeyAcceptValue()	{ return "Accepted Value"; }

    od_int64		minbodysize_;
    Interval<float>	range_;
    float		rejectvalue_;
    AcceptOutput	acceptoutput_;
    float		acceptvalue_;
    Connectivity	connectivity_;
};

} // namespace VolProc
