#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "position.h"

namespace Well
{

/*!\brief Well track */

mExpClass(Well) Track : public DahObj
{
public:

    typedef Coord3		PosType;
    typedef TypeSet<PosType>	PosSetType;

			Track(const char* nm=0);
			~Track();
			mDeclMonitorableAssignment(Track);
			mDeclInstanceCreatedNotifierAccess(Track);

    Coord3		pos(PointID) const;
    Coord3		posByIdx(IdxType) const;
    Coord3		firstPos() const;
    Coord3		lastPos() const;
    Coord3		getPos(ZType d_ah) const;
    mImplSimpleMonitoredGet(zIsTime,bool,zistime_)
    mImplSimpleMonitoredGet(getAllPos,PosSetType,pos_)
    ZType		getKbElev() const;
    ZType		td() const;
    Interval<double>	zRangeD() const;
    Interval<float>	zRange() const; //!< returns (0, 0) for empty track
    bool		alwaysDownward() const;

    ZType		getDahForTVD(double,ZType prevdah=mUdf(float)) const;
    ZType		getDahForTVD(ZType,ZType prevdah=mUdf(float)) const;
			//!< Non-unique. previous DAH may be helpful
			//!< Don't use if track is in time
    ZType		nearestDah(const Coord3&) const;
			// If zIsTime() z must be time

    void		toTime(const Data&);

	// Track building. insertPoint will find 'best' position.
    PointID		insertPoint(Coord3);
    PointID		insertPoint(Coord,float z);
    PointID		addPoint(Coord3,float dah=mUdf(float));
    PointID		addPoint(Coord,float z,float dah=mUdf(float));
			//!< Point must be further down track. No checks.
    void		setPoint(PointID,const Coord3&);
    void		setPoint(PointID,const Coord&,float z);

    virtual void	getData(ZSetType&,ValueSetType&) const;

protected:

    PosSetType		pos_;
    bool		zistime_;

    virtual bool	doSet(IdxType,ValueType);
    virtual PointID	doInsAtDah(ZType,ValueType);
    virtual ValueType	gtVal(IdxType) const;
    virtual void	removeAux( IdxType i )	{ pos_.removeSingle(i); }
    virtual void	eraseAux()		{ pos_.erase(); }

    void		doSetPoint(IdxType,const Coord3&);
    PointID		addPt(ZType,const Coord3&,AccessLockHandler*);
    PointID		insPt(IdxType,ZType,const Coord3&,AccessLockHandler*);
    PointID		insAfterIdx(IdxType,const Coord3&,
					AccessLockHandler&);
    Coord3		coordAfterIdx(ZType,IdxType) const;
    Interval<double>	gtZRangeD() const;

    friend class	TrackSampler;
    friend class	TrackIter;

};


/*!\brief Well track iterator. */

mExpClass(Well) TrackIter : public DahObjIter
{
public:
			TrackIter(const Track&,bool start_at_end=false);
			TrackIter(const TrackIter&);

    const Track&	track() const;
    Coord3		pos() const;

};



} // namespace Well

#endif
