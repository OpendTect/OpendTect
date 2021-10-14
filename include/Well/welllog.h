#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "ranges.h"
#include "iopar.h"
#include "unitofmeasure.h"
#include "propertyref.h"

namespace Well
{

/*!\brief Well log

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

mExpClass(Well) Log : public RefCount::Referenced
		    , public DahObj
{
public:

			Log(const char* nm=nullptr);
			mDeclMonitorableAssignment(Log);
			mDeclInstanceCreatedNotifierAccess(Log);

    void		setValue(PointID,ValueType);
    void		setValue(idx_type,ValueType);
    void		addValue(ZType,ValueType);

    void		getValues(ValueSetType&) const;
    virtual void	getData(ZSetType&,ValueSetType&) const;
    void		setValues(const ValueSetType&);
    void		setData(const ZSetType&,const ValueSetType&);
			// Make sure DAH values are sorted (asc or desc)

    void		removeTopBottomUdfs();

    mImplSimpleMonitoredGetSet(inline,unitMeasLabel,setUnitMeasLabel,
				BufferString,unitmeaslbl_,cParsChange())
    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,cParsChange())
    const char*		mnemLabel() const;
    void		setMnemLabel( const char* );
    const Mnemonic*	mnemonic() const;
    const UnitOfMeasure* unitOfMeasure() const;
    void		applyUnit(const UnitOfMeasure*);
    void		convertTo(const UnitOfMeasure*);
    PropertyRef::StdType propType() const;
    mImplSimpleMonitoredGet(valueRange,Interval<ValueType>,valrg_)
    mImplSimpleMonitoredGet(valsAreCodes,bool,valsarecodes_)

    static const char*	sKeyMnemLbl();
    static const char*	sKeyUnitLbl();
    static const char*	sKeyHdrInfo();
    static const char*	sKeyStorage();
    static const char*	sKeyDahRange();
    static const char*	sKeyLogRange();

protected:

			~Log();

    ValueSetType	vals_;
    BufferString	unitmeaslbl_;
    BufferString	mnemlbl_;
    IOPar		pars_;
    Interval<ValueType>	valrg_;
    bool		valsarecodes_;

    virtual bool	doSet(idx_type,ValueType);
    virtual PointID	doInsAtDah(ZType,ValueType);
    virtual ValueType	gtVal( idx_type idx ) const { return vals_[idx]; }
    virtual void	removeAux( idx_type idx )   { vals_.removeSingle(idx); }
    virtual void	eraseAux()		    { vals_.erase(); }

    ValueType		gtVal(ZType,int&) const;
    void		stVal(idx_type,ValueType);

    void		redoValStats();
    void		updValStats(ValueType);
    void		ensureAscZ();

};


/*!\brief Well Log iterator. */

mExpClass(Well) LogIter : public DahObjIter
{
public:

			LogIter(const Log&,bool start_at_end=false);
			LogIter(const LogIter&);

    const Log&		log() const;

};

} // namespace Well
