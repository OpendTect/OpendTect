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
#include "propertyref.h"

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
			, range_(mUdf(float),-mUdf(float))
			, iscode_(false)			    {}
			Log( const Log& t )
			: DahObj("")			{ *this = t; }
    Log&		operator =(const Log&);

    float		value( int idx ) const		{ return vals_[idx]; }
    void		setValue(int idx,float val);

    float		getValue(float,bool noudfs=false) const;
    void		addValue(float dh,float val);
			//!< addition must always ascend or descend
    void		updateAfterValueChanges();
			//!< call it upon any change of value(s)
    void		ensureAscZ();
			// Do this after adding values when Z may be reversed
    bool		insertAtDah(float dh,float val);
    void		removeTopBottomUdfs();

    Interval<float>&	valueRange()			{ return range_; }
    const Interval<float>& valueRange() const		{ return range_; }
    Interval<float>&	dahRange()			{ return dahrange_; }
    const Interval<float>& dahRange() const		{ return dahrange_; }

    const char*		unitMeasLabel() const		{ return unitmeaslbl_;}
    const UnitOfMeasure* unitOfMeasure() const;
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    void		convertTo(const UnitOfMeasure*);
    PropertyRef::StdType propType() const;
    bool		isCode() const			{ return iscode_; }
			//!< log values are all integers stored as floats
    static const char*	sKeyUnitLbl();
    static const char*	sKeyHdrInfo();
    static const char*	sKeyStorage();
    static const char*	sKeyDahRange();

    float*		valArr()			{ return vals_.arr(); }
    const float*	valArr() const			{ return vals_.arr(); }

    IOPar&		pars()				{ return pars_; }
    const IOPar&	pars() const			{ return pars_; }

protected:

    TypeSet<float>	vals_;
    Interval<float>	range_;
    Interval<float>	dahrange_;
    BufferString	unitmeaslbl_;
    bool		iscode_;
    IOPar		pars_;

    void		removeAux( int idx )	{ vals_.removeSingle(idx); }
    void		eraseAux()		{ vals_.erase(); }
    float		gtVal(float,int&) const;

};


mExpClass(Well) LogInfo: public NamedObject
{mODTextTranslationClass(Well::LogInfo)
public:

			LogInfo(const char* nm);

    BufferString	logunit_;
    Interval<float>	dahrg_;
};


mExpClass(Well) LogInfoSet : public ObjectSet<Well::LogInfo>
{
public:
    void			getNames(BufferStringSet&) const;
    void			getUnits(BufferStringSet&) const;

    const Well::LogInfo*	getByName(const char* lognm) const;
    BufferString		getUnit(const char* lognm) const;
    Interval<float>		dahRange(const char* lognm) const;
    bool			logIsPresent(const char* lognm) const;
};



} // namespace Well

#endif
