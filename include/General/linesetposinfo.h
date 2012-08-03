#ifndef linesetposinfo_h
#define linesetposinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2005 / Mar 2008
 RCS:		$Id: linesetposinfo.h,v 1.5 2012-08-03 13:00:24 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posinfo2d.h"
class BinIDValueSet;


namespace PosInfo
{

/*!\brief Position info for a set of 2D lines */

mClass(General) LineSet2DData
{
public:

    virtual		~LineSet2DData()	{ deepErase(data_); }

    Line2DData&		addLine(const char*);
    int			nrLines() const		{ return data_.size(); }
    const BufferString&	lineName( int idx ) const
    			{ return data_[idx]->lnm_; }
    const Line2DData&	lineData( int idx ) const
    			{ return data_[idx]->pos_; }
    const Line2DData*	getLineData(const char*) const;
    void		removeLine(const char*);

    struct IR
    {
			IR() : posns_(0)	{}
			~IR(); //		{ delete posns_; }

	BufferString	lnm_;
	BinIDValueSet*	posns_;
    };
    void		intersect(const BinIDValueSet&,ObjectSet<IR>&) const;

protected:

    struct Info
    {
	BufferString	lnm_;
	Line2DData	pos_;
    };

    ObjectSet<Info>	data_;

    Info*		findLine(const char*) const;

};

} // namespace PosInfo

#endif

