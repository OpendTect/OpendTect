#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007 / Mar 2017
________________________________________________________________________

-*/

#include "coltabmappersetup.h"
#include "datadistribution.h"


namespace ColTab
{

/*!\brief Maps data values to color sequence positions: [0,1].

  Beware that the Mapper *requires* a data distribution if you want to map
  anything else than [0,1] to [0,1]. The Mapper will no longer scan your data,
  that needs to be done before using the Mapper. Using the
  [RangeLimited]DataDistributionExtracter may be helpful for that.

  If setup().nrSegs() > 0, the mapper will return the centers of the
  segments only. For example, if nsegs_ == 3, only positions returned are
  1/6, 3/6 and 5/6.

*/

mExpClass(General) Mapper : public SharedObject
{
public:

    typedef MapperSetup::RangeType	RangeType;
    typedef DataDistribution<ValueType>	DistribType;

			Mapper();	//!< default maps [0,1] to [0,1]
			Mapper(RangeType);
			Mapper(const MapperSetup&);
			mDeclMonitorableAssignment(Mapper);

    MapperSetup&	setup()			{ return *setup_; }
    const MapperSetup&	setup() const		{ return *setup_; }
    DistribType&	distribution()		{ return *distrib_; }
    const DistribType&	distribution() const	{ return *distrib_; }

    RangeType		getRange() const;	//!< guaranteed not udf

    PosType		relPosition(ValueType) const; //!< [0,1] in bar
    PosType		seqPosition(ValueType) const; //!< [0,1] in col seq
    int			colIndex(ValueType,int nrcolors) const;

    static ChangeType	cMappingChange()	{ return 2; }

    void		transferSubObjNotifsTo(const Mapper&) const;

    static bool		isNearZeroSymmetry(const RangeType&);

protected:

			~Mapper();

    RefMan<MapperSetup>	setup_;
    RefMan<DistribType>	distrib_;

    void		setNotifs();
    static PosType	getLinRelPos(const RangeType&,ValueType);
    PosType		getHistEqRelPos(const RangeType&,ValueType) const;
    void		determineRange() const;

    void		setupChgCB(CallBacker*);
    void		distribChgCB(CallBacker*);

};


} // namespace ColTab
