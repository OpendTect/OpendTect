#ifndef welllog_h
#define welllog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllog.h,v 1.8 2004-02-12 15:49:46 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "uidobj.h"
#include "ranges.h"

namespace Well
{

class Log : public ::UserIDObject
{
public:

			Log( const char* nm=0 )
			: ::UserIDObject(nm)
			, range_(mUndefValue,-mUndefValue)	{}

    int			size() const			{ return val_.size(); }
    float		value(int idx) const		{ return val_[idx]; }
    float		dah(int idx) const		{ return dah_[idx]; }

    float		getValue(float) const;
    void		addValue(float z,float val);
    			//!< z must be > last dah. No checks.
    void		removeValue(int idx)
			{ dah_.remove(idx); val_.remove(idx); }

    const Interval<float>& range() const		{ return range_; }
    Interval<float>	selrange;

    const char*		unitMeasLabel() const		{ return unitmeaslbl_; }
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    static const char*	sKeyUnitLbl;

protected:

    TypeSet<float>	dah_;
    TypeSet<float>	val_;
    Interval<float>	range_;
    BufferString	unitmeaslbl_;

};


}; // namespace Well

#endif
