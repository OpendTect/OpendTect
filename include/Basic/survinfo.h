#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Features for sets of data
 RCS:		$Id: survinfo.h,v 1.8 2001-05-18 13:37:04 bert Exp $
________________________________________________________________________

-*/
 
 
#include <binid2coord.h>
#include <uidobj.h>
#include <binidselimpl.h>
class ascostream;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline and coordinates.

*/

class SurveyInfo : public UserIDObject
{

    friend class		EdSurvey;
    friend class		EdSurveyInfo;
    friend const SurveyInfo&	SI();

public:

			SurveyInfo(const SurveyInfo&);

    const BinIDRange&	range() const		{ return range_; }
    void		setRange(const BinIDRange&);
    const BinID&	step() const		{ return step_; }
    void		setStep(const BinID&);
    const Interval<double>& zRange() const	{ return zrange_; }
    void		setZRange(const Interval<double>&);

    bool		rangeUsable() const
			{ return range_.start.inl && range_.stop.inl
			      && range_.start.crl && range_.stop.crl; }
    bool		zRangeUsable() const
			{ return !mIS_ZERO(zrange_.width()); }

    void		snap(BinID&,const BinID& direction) const;
			// 0 : auto; -1 round downward, 1 round upward

    inline bool		validTransform() const
			{ return b2c.isValid(); }
    inline Coord	transform( const BinID& b ) const
			{ return b2c.transform(b); }
    BinID		transform(const Coord&) const;
    inline void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
			//!< returns error message or null on success

    const BinID2Coord&	binID2Coord() const	{ return b2c; }

    Coord		minCoord() const;

private:

			SurveyInfo(const char*);
    int			write(const char*) const;

    UserIDString	dirname;
    void		setTr(BinID2Coord::BCTransform&,const char*);
    void		putTr(const BinID2Coord::BCTransform&,	
				ascostream&,const char*) const;

    BinID2Coord		b2c;
    BinIDRange		range_;
    BinID		step_;
    Interval<double>	zrange_;

    BinID		set3binids[3];
    Coord		set3coords[3];

    static SurveyInfo*	theinst_;

};


#endif
