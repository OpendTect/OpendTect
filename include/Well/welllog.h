#ifndef welllog_h
#define welllog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllog.h,v 1.11 2004-05-27 10:07:10 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "welldahobj.h"
#include "ranges.h"

namespace Well
{

class Log : public DahObj
{
public:

			Log( const char* nm=0 )
			: DahObj(nm)
			, range_(mUndefValue,-mUndefValue)
			, displogrthm_(false)		{}
			Log( const Log& t )
			: DahObj("")			{ *this = t; }
    Log&		operator =(const Log&);

    float		value( int idx ) const		{ return val_[idx]; }

    float		getValue(float) const;
    void		addValue(float z,float val);
    			//!< z must be > last dah. No checks.

    const Interval<float>& valueRange() const		{ return range_; }
    void		setSelValueRange(const Interval<float>&);
    const Interval<float>& selValueRange() const	{ return selrange_; }
    bool		dispLogarithmic() const		{ return displogrthm_; }
    void		setDispLogarithmic( bool yn )	{ displogrthm_ = yn; }

    const char*		unitMeasLabel() const		{ return unitmeaslbl_; }
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    static const char*	sKeyUnitLbl;

protected:

    TypeSet<float>	val_;
    Interval<float>	range_;
    Interval<float>	selrange_;
    BufferString	unitmeaslbl_;
    bool		displogrthm_;

    void		removeAux( int idx )		{ val_.remove(idx); }
    void		eraseAux()			{ val_.erase(); }

};


}; // namespace Well

#endif
