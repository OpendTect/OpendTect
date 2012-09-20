#ifndef voxelconnectivityfilter_h
#define voxelconnectivityfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "voxelconnectivityfiltermod.h"
#include "volprocchain.h"
#include "enums.h"
#include "enums.h"

namespace VolProc
{

mClass(VoxelConnectivityFilter) VoxelConnectivityFilter : public Step
{
public:

			mDefaultFactoryInstantiation( Step,
				VoxelConnectivityFilter,
				"VoxelConnectivityFilter",
				"Voxel Connectivity Filter" );

    enum		Connectivity { Faces, Edges, Corners };
    			DeclareEnumUtils( Connectivity );

    enum		AcceptOutput { Ranking, BodySize, Value, Transparent };
    			DeclareEnumUtils( AcceptOutput );

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


    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		needsInput() const 		{ return true; }
    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		needsFullVolume() const		{ return true; }
    bool		areSamplesIndependent() const	{ return false; }
    const char*		errMsg() const		{ return errmsg_.str(); }

    Task*		createTask();

protected:

		    VoxelConnectivityFilter();
		    ~VoxelConnectivityFilter();

    static const char*	sKeyRange()		{ return "Range"; }
    static const char*	sKeyConnectivity()	{ return "Connectivity"; }
    static const char*	sKeyMinimumSize()	{ return "Minimum size"; }
    static const char*	sKeyRejectValue()	{ return "Rejected Value"; }
    static const char*	sKeyAcceptOutput()	{ return "Accepted Output"; }
    static const char*	sKeyAcceptValue()	{ return "Accepted Value"; }

    FixedString		errmsg_;
    od_int64		minbodysize_;
    Interval<float>	range_;
    float		rejectvalue_;
    AcceptOutput	acceptoutput_;
    float		acceptvalue_;
    Connectivity	connectivity_;
};

}; //namespace

#endif

