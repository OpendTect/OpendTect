#ifndef welllog_h
#define welllog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "welldahobj.h"
#include "ranges.h"
#include "iopar.h"
#include "unitofmeasure.h"

namespace Well
{

/*!
\brief Well log
 
  No regular sampling required, as in all DahObjs. 

  Logs can contain undefined points and sections (washouts etc.) - they will
  never have undefined dah values though. If you can't handle undef values,
  use getValue( dah, true ).

  Log are imported 'as is', making this one of the rare non-SI value
  objects in OpendTect. The unit of measure label you get may be able to
  uncover what the actual unit of measure is.

  The IOPar pars() will be retrieved and stored with each log; it is not
  really used; moreover, it's intended for plugins to dump their extra info
  about this log.
*/

mExpClass(Well) Log : public DahObj
{
public:

			Log( const char* nm=0 )
			: DahObj(nm)
			, range_(mUdf(float),-mUdf(float))	{}
			Log( const Log& t )
			: DahObj("")			{ *this = t; }
    Log&		operator =(const Log&);

    float		value( int idx ) const		{ return val_[idx]; }

    float		getValue(float,bool noudfs=false) const;
    void		addValue(float dh,float val);
    			//!< addition must always ascend or descend
    void		ensureAscZ();
    			// Do this after adding values when Z may be reversed
    bool		insertAtDah(float dh,float val);
    void		removeTopBottomUdfs();

    Interval<float>&	valueRange() 			{ return range_; }
    const Interval<float>& valueRange() const 		{ return range_; }

    const char*		unitMeasLabel() const		{ return unitmeaslbl_;}
    const UnitOfMeasure* unitOfMeasure() const;
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    void		convertTo(const UnitOfMeasure*);
    static const char*	sKeyUnitLbl();
    static const char*	sKeyHdrInfo();
    static const char*	sKeyStorage();

    float*		valArr()			{ return val_.arr(); }
    const float*	valArr() const			{ return val_.arr(); }

    IOPar&		pars()				{ return pars_; }
    const IOPar&	pars() const			{ return pars_; }

protected:

    TypeSet<float>	val_;
    Interval<float>	range_;
    BufferString	unitmeaslbl_;
    IOPar		pars_;

    void		removeAux( int idx )	{ val_.removeSingle(idx); }
    void		eraseAux()		{ val_.erase(); }
    float		gtVal(float,int&) const;

};

}; // namespace Well

#endif

