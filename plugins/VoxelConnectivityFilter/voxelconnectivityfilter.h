#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2011
________________________________________________________________________

-*/

#include "voxelconnectivityfiltermod.h"
#include "volprocchain.h"
#include "enums.h"
#include "enums.h"

namespace VolProc
{

mExpClass(VoxelConnectivityFilter) VoxelConnectivityFilter : public Step
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


    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

private:

    virtual uiString	errMsg() const			{ return errmsg_; }
    virtual ReportingTask*	createTask();

    virtual bool	needsFullVolume() const		{ return true; }
    virtual bool	canInputAndOutputBeSame() const	{ return true; }
    virtual bool	areSamplesIndependent() const	{ return false; }
    virtual bool	needsInput() const		{ return true; }

			VoxelConnectivityFilter();
			~VoxelConnectivityFilter();

    virtual od_int64	extraMemoryUsage(OutputSlotID,
					 const TrcKeyZSampling&) const;

    od_int64		minbodysize_;
    Interval<float>	range_;
    float		rejectvalue_;
    AcceptOutput	acceptoutput_;
    float		acceptvalue_;
    Connectivity	connectivity_;

    static const char*	sKeyRange()		{ return "Range"; }
    static const char*	sKeyConnectivity()	{ return "Connectivity"; }
    static const char*	sKeyMinimumSize()	{ return "Minimum size"; }
    static const char*	sKeyRejectValue()	{ return "Rejected Value"; }
    static const char*	sKeyAcceptOutput()	{ return "Accepted Output"; }
    static const char*	sKeyAcceptValue()	{ return "Accepted Value"; }

};

}; //namespace
