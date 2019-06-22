#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/02/2001
________________________________________________________________________

-*/

#include "generalmod.h"
#include "ranges.h"
#include "string2.h"
#include "undefval.h"
#include "bufstringset.h"
#include "position.h"
#include "typeset.h"
#include "uistrings.h"

class EnumDef;

/*!\brief Data type. */

mExpClass(General) InpDataType
{ mODTextTranslationClass(InpDataType)
public:

    enum	Rep  { intTp, int64Tp, floatTp, doubleTp, boolTp, stringTp };
    enum	Form { normal, interval, filename, position, list };

		InpDataType( Rep tp, Form frm=normal )
		    : rep_( tp ), form_(frm) {}

    inline Rep	rep() const		{ return rep_; }
    inline Form	form() const		{ return form_; }

    bool	operator==( const InpDataType& oth ) const
		    { return (rep_==oth.rep_) && (form_==oth.form_); }
    bool	operator!=( const InpDataType& oth ) const
		    { return ! (oth == *this); }


protected:

    Rep		rep_;
    Form	form_;

};


/*!\brief InpDataType implementation.  */

template<class T>
mClass(General) InpDataTypeImpl : public InpDataType
{ mODTextTranslationClass(InpDataTypeImpl)
public:


			InpDataTypeImpl( Form frm=normal )
			    : InpDataType( rep__( (T)0 ), frm ) {}
protected:

    inline Rep		rep__( int i )    const	{ return intTp; }
    inline Rep		rep__( od_int64 i ) const { return int64Tp; }
    inline Rep		rep__( float f )  const	{ return floatTp; }
    inline Rep		rep__( double d ) const	{ return doubleTp; }
    inline Rep		rep__( bool b )   const	{ return boolTp; }
    inline Rep		rep__( const char* s ) const { return stringTp; }

};




/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/
mExpClass(General) DataInpSpec
{ mODTextTranslationClass(DataInpSpec)
public:


			DataInpSpec(InpDataType);
			DataInpSpec(const DataInpSpec&);
    virtual		~DataInpSpec()			{}

    InpDataType		type() const;

    virtual DataInpSpec* clone() const			=0;
    virtual int	nElems() const			{ return 1; }

    virtual bool	isUndef( int idx=0 ) const	=0;

    virtual bool	hasLimits() const		{ return false; }
    virtual bool	isInsideLimits(int idx=0) const;

    virtual const char*	text( int idx=0 ) const		=0;
    virtual bool	setText(const char*,int idx=0)	=0;

    void		fillPar(IOPar&) const;
			/*!Saves the _values_ (from text()) */
    bool		usePar(const IOPar&);
			/*!Sets the _values_ (with setText()) */

    virtual int		getIntValue(int idx=0) const;
    virtual od_int64	getInt64Value(int idx=0) const;
    virtual double	getDValue(int idx=0) const;
    virtual float	getFValue(int idx=0) const;
    virtual bool	getBoolValue(int idx=0) const;

    virtual void	setValue(int i,int idx=0);
    virtual void	setValue(od_int64 i,int idx=0);
    virtual void	setValue(double d,int idx=0);
    virtual void	setValue(float f,int idx=0);
    virtual void	setValue(bool b,int idx=0);

    virtual int		getDefaultIntValue(int idx=0) const;
    virtual double	getDefaultDValue(int idx=0) const;
    virtual float	getDefaultFValue(int idx=0) const;
    virtual bool	getDefaultBoolValue(int idx=0) const;
    virtual const char*	getDefaultStringValue(int idx=0) const;

    virtual void	setDefaultValue(int i,int idx=0)		{}
    virtual void	setDefaultValue(double d,int idx=0)		{}
    virtual void	setDefaultValue(float f,int idx=0)		{}
    virtual void	setDefaultValue(bool b,int idx=0)		{}
    virtual void	setDefaultValue(const char* s,int idx=0)	{}

    virtual const char*	name(int idx=0) const;
    virtual DataInpSpec& setName(const char*,int idx=0);

protected:

    void		setType( InpDataType t );
    InpDataType		tp_;
    bool		prefempty_;

    TypeSet<int>	nameidxs_;
    BufferStringSet	names_;

private:

    static const char*	valuestr;

};


#define mDefDISSetValBaseClassImpl(typ) \
    virtual void	setValue( typ val, int idx=0 ) \
			{ DataInpSpec::setValue(val,idx); }
#define mDefDISSetDefValBaseClassImpl(typ) \
    virtual void	setDefaultValue( typ val, int idx=0 ) \
			{ DataInpSpec::setDefaultValue(val,idx); }


/*! \brief Specifications for numerical inputs that may or may not have limits

    If the input must lie in a certain range, this range can be obtainted by
    Limits() and set by... setLimits().

*/
template <class T>
mClass(General) NumInpSpec : public DataInpSpec
{ mODTextTranslationClass(NumInpSpec)
public:
			NumInpSpec()
			    : DataInpSpec( InpDataTypeImpl<T>() )
			    , limits_(0)
			    { mSetUdf( value_ ); mSetUdf( defaultvalue_ ); }
			NumInpSpec( T val )
			    : DataInpSpec( InpDataTypeImpl<T>() )
			    , limits_(0)
			    , value_(val)
			    { mSetUdf( defaultvalue_ ); }
			NumInpSpec( T val, T start, T stop=mUdf(T), T step=1)
			    : DataInpSpec(InpDataTypeImpl<T>())
			    , limits_(new StepInterval<T>(start,stop,step))
			    , value_(val)
			    { mSetUdf( defaultvalue_ ); }
			NumInpSpec( T val, const StepInterval<T>& intv )
			    : DataInpSpec(InpDataTypeImpl<T>())
			    , limits_(new StepInterval<T>(intv))
			    , value_(val)
			    { mSetUdf( defaultvalue_ ); }
			NumInpSpec(const NumInpSpec<T>&);
			~NumInpSpec()			{ delete limits_; }

    virtual NumInpSpec<T>* clone() const
			    { return new NumInpSpec<T>( *this ); }

    virtual bool	isUndef( int idx=0 ) const
			    { return mIsUdf(value_); }

    virtual bool	setText( const char* s, int idx=0 )
			    { value_ = Conv::to<T>(s); return mIsUdf(value_); }

    virtual int		getIntValue(int idx=0) const { return (int)value(); }
    virtual od_int64	getInt64Value(int idx=0) const
						{ return (od_int64)value(); }
    virtual double	getDValue(int idx=0) const    { return value(); }
    virtual float	getFValue(int idx=0) const   { return (float)value(); }

    virtual int		getDefaultIntValue(int idx=0) const
			{ return (int)defaultValue(); }
    virtual double	getDefaultDValue(int idx=0) const
			{ return (double)defaultValue(); }
    virtual float	getDefaultFValue(int idx=0) const
			{ return (float)defaultValue(); }

    virtual void	setDefaultValue( int val, int idx=0 )
			{ defaultvalue_ = (T)val; }
    virtual void	setDefaultValue( double val, int idx=0 )
			{ defaultvalue_ = (T)val; }
    virtual void	setDefaultValue( float val, int idx=0 )
			{ defaultvalue_ = (T)val; }
			mDefDISSetDefValBaseClassImpl(bool)
			mDefDISSetDefValBaseClassImpl(const char*)
    T			value() const
			{
			    if ( mIsUdf(value_) ) return mUdf(T);
			    return value_;
			}

    T			defaultValue() const
			{
			    if ( mIsUdf(defaultvalue_) ) return mUdf(T);
			    return defaultvalue_;
			}

    virtual const char*	text( int idx=0 ) const
			{
			    if ( isUndef() )	return "";
			    else		return toString( value() );
			}

    virtual bool	hasLimits() const		{ return limits_; }
    virtual bool	isInsideLimits( int idx=0 ) const
			{
			    if ( !limits_ ) return true;
			    if ( !isUndef(idx) )
				return limits_->includes( value(), true );;

			    const bool startudf = mIsUdf(limits_->start);
			    const bool stopudf = mIsUdf(limits_->stop);
			    if ( startudf && stopudf ) return true;
			    if ( startudf )
				return value() < limits_->stop;
			    if ( stopudf )
				return value() > limits_->start;
			    return false;
			}

    const StepInterval<T>* limits() const		{ return limits_; }
    NumInpSpec<T>&	setLimits( const Interval<T>& intv )
			{ return setLimits(
				StepInterval<T>(intv.start,intv.stop,1) ); }
    NumInpSpec<T>&	setLimits( const StepInterval<T>& r )
			{
			    delete limits_;
			    limits_ = new StepInterval<T>( r );
			    return *this;
			}

protected:

    T			value_;
    T			defaultvalue_;

    StepInterval<T>*	limits_;
};


template <class T>
NumInpSpec<T>::NumInpSpec( const NumInpSpec<T>& nis )
    : DataInpSpec( nis )
    , limits_( nis.limits_ ? new StepInterval<T>(*nis.limits_) : 0 )
    , value_( nis.value_ )
    , defaultvalue_( nis.defaultvalue_ )
{}


typedef NumInpSpec<int>		IntInpSpec;
typedef NumInpSpec<od_int64>	Int64InpSpec;
typedef NumInpSpec<float>	FloatInpSpec;
typedef NumInpSpec<double>	DoubleInpSpec;


/*! \brief Specifications for numerical intervals

    Intervals consist of a start + stop value and optionally a step value, in
    which case the Interval is a StepInterval.

    For each of the interval components start, stop and step, separate
    limits can be set.

*/
template <class T>
mClass(General) NumInpIntervalSpec : public DataInpSpec
{ mODTextTranslationClass(NumInpIntervalSpec)
public:
			NumInpIntervalSpec( bool withstep=false )
			    : DataInpSpec(InpDataTypeImpl<T>(
						    InpDataType::interval))
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
			    , symm_(false)
			    , wsymm_(false)
			    , interval_( withstep ?  new StepInterval<T>(
							mUdf(T),
							mUdf(T),
							mUdf(T) )
						  : new Interval<T>(
							mUdf(T),
							mUdf(T) ) )
			    , defaultinterval_( withstep ? new StepInterval<T>(
								mUdf(T),
								mUdf(T),
								mUdf(T) )
							 : new Interval<T>(
								mUdf(T),
								mUdf(T) ) )
			{}

			NumInpIntervalSpec( const Interval<T>& interval )
			    : DataInpSpec( InpDataTypeImpl<T>(
						InpDataType::interval) )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
			    , symm_(false)
			    , wsymm_(false)
			    , interval_( interval.clone() )
			    , defaultinterval_( hasStep() ? new StepInterval<T>(
								mUdf(T),
								mUdf(T),
								mUdf(T) )
							 : new Interval<T>(
								mUdf(T),
								mUdf(T) ) )
			{}

			NumInpIntervalSpec( const NumInpIntervalSpec<T>& o )
			    : DataInpSpec( o )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
			    , symm_(o.isSymmetric())
			    , wsymm_(o.withSymmetric())
			    , interval_( o.interval_ ? o.interval_->clone() : 0)
			    , defaultinterval_( o.defaultinterval_ ?
						o.defaultinterval_->clone() : 0)
			{
			    if ( o.limits(0) ) setLimits( *o.limits(0), 0 );
			    if ( o.limits(1) ) setLimits( *o.limits(1), 1 );
			    if ( o.limits(2) ) setLimits( *o.limits(2), 2 );
			}

			~NumInpIntervalSpec()
			{
			    delete interval_;
			    delete defaultinterval_;
			    delete startlimits_;
			    delete stoplimits_;
			    delete steplimits_;
			}

    virtual NumInpIntervalSpec<T>* clone() const
			{ return new NumInpIntervalSpec<T>( *this ); }

    virtual int		nElems()  const	{ return hasStep() ? 3 : 2; }
    inline bool		hasStep() const	{ return stpi(); }

    virtual bool	isUndef( int idx=0 ) const
			{
			    if ( !interval_ ) return true;
			    return mIsUdf( value_(idx) );
			}

    virtual void	setValue( const Interval<T>& intval )
			{
			    if ( interval_ ) delete interval_;
			    interval_ = intval.clone();
			}
			mDefDISSetValBaseClassImpl(int)
			mDefDISSetValBaseClassImpl(od_int64)
			mDefDISSetValBaseClassImpl(bool)
			mDefDISSetValBaseClassImpl(float)
			mDefDISSetValBaseClassImpl(double)

    virtual void	setDefaultValue( const Interval<T>& defaultintval )
			{
			    if ( defaultinterval_ ) delete defaultinterval_;
			    defaultinterval_ = defaultintval.clone();
			}
			mDefDISSetDefValBaseClassImpl(int)
			mDefDISSetDefValBaseClassImpl(float)
			mDefDISSetDefValBaseClassImpl(double)
			mDefDISSetDefValBaseClassImpl(bool)
			mDefDISSetDefValBaseClassImpl(const char*)

    virtual bool	setText( const char* s, int idx=0 )
			{
			    T* ptval = pt_value_( idx );
			    if ( !ptval )
				return false;
			    *ptval = Conv::to<T>( s );
			    return mIsUdf( *ptval );
			}

    T			value( int idx=0 ) const
			{
			    if ( isUndef(idx) ) return mUdf(T);
			    return value_(idx);
			}

    virtual int		getIntValue(int idx=0) const { return (int)value(idx); }
    virtual od_int64	getInt64Value(int idx=0) const
						{ return (od_int64)value(idx); }
    virtual double	getDValue(int idx=0) const   { return value(idx); }
    virtual float	getFValue(int idx=0) const { return (float)value(idx); }

    T			defaultValue( int idx=0 ) const
			{
			    if ( !defaultinterval_ ) return mUdf(T);
			    return defaultvalue_(idx);
			}

    virtual int		getDefaultIntValue(int idx=0) const
			{ return (int)defaultValue(idx); }
    virtual double	getDefaultDValue(int idx=0) const
			{ return (double)defaultValue(idx); }
    virtual float	getDefaultFValue(int idx=0) const
			{ return (float)defaultValue(idx); }

    virtual const char*	text( int idx=0 ) const
			{
			    if ( isUndef(idx) ) return "";
			    return toString( value(idx));
			}

    virtual bool	hasLimits() const
			{ return startlimits_||stoplimits_||steplimits_; }
    virtual bool	isInsideLimits( int idx=0 ) const
			{
			    const StepInterval<T>* lims = limits(idx);
			    if ( !lims ) return true;
			    if ( !isUndef(idx) )
				return lims->includes( value(idx), true );

			    const bool startudf = mIsUdf(lims->start);
			    const bool stopudf = mIsUdf(lims->stop);
			    if ( startudf && stopudf ) return true;
			    if ( startudf )
				return value(idx) < lims->stop;
			    if ( stopudf )
				return value(idx) > lims->start;
			    return false;
			}


			/*! \brief gets limits for interval components.

			    idx =  0: returns start limits
			    idx =  1: returns stop limits
			    idx =  2: returns step limits
			*/
    const StepInterval<T>* limits( int idx=0 ) const
			{
			    switch( idx )
			    {
			    default:
			    case 0 : return startlimits_;
			    case 1 : return stoplimits_;
			    case 2 : return steplimits_;
			    }
			}


			/*! \brief sets limits for interval components.

			    idx =  0: sets start limits
			    idx =  1: sets stop limits
			    idx =  2: sets step limits

			    idx = -1: sets start and stop limits
			    idx = -2: sets start, stop and step limits
			*/
    NumInpIntervalSpec&	setLimits( const StepInterval<T>& r, int idx=-1 )
			{
			    if ( idx<0 || idx==0 )
			    {
				delete startlimits_;
				startlimits_ = new StepInterval<T>( r );
			    }
			    if ( idx<0 || idx==1 )
			    {
				delete stoplimits_;
				stoplimits_ = new StepInterval<T>( r );
			    }
			    if ( idx==-2 || idx==2 )
			    {
				delete steplimits_;
				steplimits_ = new StepInterval<T>( r );
			    }

			    return *this;
			}
    NumInpIntervalSpec&	setLimits( const Interval<T>& r, int idx=-1 )
			{
			    return setLimits(
				StepInterval<T>(r.start,r.stop,1), idx );
			}

    void		setSymmetric( bool yn )
			{ symm_ = yn; wsymm_ = true; }
    bool		isSymmetric() const		{ return symm_; }
    void		setWithSymmetric( bool yn )	{ wsymm_ = yn; }
    bool		withSymmetric() const		{ return wsymm_; }

protected:

    Interval<T>*	interval_;
    Interval<T>*	defaultinterval_;

    StepInterval<T>*	startlimits_;
    StepInterval<T>*	stoplimits_;
    StepInterval<T>*	steplimits_;

    bool		symm_;
    bool		wsymm_;

    T			value_( int idx=0 ) const
			{
			    if ( pt_value_(idx) ) return *pt_value_(idx);
			    return mUdf(T);
			}

    T			defaultvalue_( int idx=0 ) const
			{
			    if ( pt_value_(idx,true) )
				return *pt_value_(idx,true);

			    return mUdf(T);
			}

    T*			pt_value_( int idx=0, bool defval=false ) const
			{
			    if ( defval )
			    {
				if ( !defaultinterval_) return 0;
				if ( idx == 0 ) return &defaultinterval_->start;
				if ( idx == 1 )  return &defaultinterval_->stop;
				if ( hasStep() ) return &stpi(defval)->step;
			    }
			    else
			    {
				if ( !interval_) return 0;
				if ( idx == 0 )	 return &interval_->start;
				if ( idx == 1 )	 return &interval_->stop;
				if ( hasStep() ) return &stpi()->step;
			    }
			    return 0;
			}

    StepInterval<T>*	stpi( bool defval=false ) const
			{
			    mDynamicCastGet(const StepInterval<T>*,si,
					defval ? defaultinterval_ : interval_)
			    return const_cast<StepInterval<T>*>( si );
			}
};


typedef NumInpIntervalSpec<int>		IntInpIntervalSpec;
typedef NumInpIntervalSpec<float>	FloatInpIntervalSpec;
typedef NumInpIntervalSpec<double>	DoubleInpIntervalSpec;


/*! \brief Specifications for character string inputs. */
mExpClass(General) StringInpSpec : public DataInpSpec
{ mODTextTranslationClass(StringInpSpec);
public:
			StringInpSpec( const char* s=0 );
    virtual bool	isUndef(int idx=0) const;

    virtual DataInpSpec* clone() const;
    const char*		text() const;

    virtual bool	setText(const char*,int idx=0) ;
    virtual const char*	text(int) const;

    void		setDefaultValue(const char*,int);
    const char*		getDefaultStringValue(int) const;

			mDefDISSetDefValBaseClassImpl(int)
			mDefDISSetDefValBaseClassImpl(float)
			mDefDISSetDefValBaseClassImpl(double)
			mDefDISSetDefValBaseClassImpl(bool)

protected:

    bool		isUndef_;
    BufferString	str_;
    BufferString	defaultstr_;

};

/*! \brief Specifications for file-name inputs.
*/
mExpClass(General) FileNameInpSpec : public StringInpSpec
{ mODTextTranslationClass(FileNameInpSpec);
public:
				FileNameInpSpec( const char* fname=0 );
    virtual DataInpSpec*	clone() const;
};


/*! \brief Specifications for boolean inputs.

This specifies a boolean input field. If the second char string is "" or NULL,
then a checkbox will be created. Otherwise two connected
radio buttons will do the job. Default will create two radio buttons "Yes" and
"No".  When calling setText("xx") on the resulting uiGenInput, it will try to
set the boolean value according to the given true/false text's or "Yes"/"No".
It does not change the underlying true/false texts.

*/


mExpClass(General) BoolInpSpec : public DataInpSpec
{ mODTextTranslationClass(BoolInpSpec);
public:
			BoolInpSpec(bool yesno,
				    const uiString& truetxt=uiStrings::sYes(),
				    const uiString& falsetxt=uiStrings::sNo(),
				    bool isset=true);
			BoolInpSpec(const BoolInpSpec&);

    virtual bool	isUndef(int idx=0) const;

    virtual DataInpSpec* clone() const;
    uiString		trueFalseTxt(bool tf=true) const;
    void		setTrueFalseTxt(bool,const uiString&);

    bool		checked() const;
    void		setChecked(bool yesno);
    virtual const char*	text(int idx=0) const;

    virtual bool	setText(const char* s,int idx=0);
    virtual bool	getBoolValue(int idx=0) const;
    virtual void	setValue(bool,int idx=0);
			mDefDISSetValBaseClassImpl(int)
			mDefDISSetValBaseClassImpl(od_int64)
			mDefDISSetValBaseClassImpl(float)
			mDefDISSetValBaseClassImpl(double)
    virtual bool	getDefaultBoolValue(int idx=0) const;
    virtual void	setDefaultValue(bool,int idx=0);
			mDefDISSetDefValBaseClassImpl(int)
			mDefDISSetDefValBaseClassImpl(float)
			mDefDISSetDefValBaseClassImpl(double)
			mDefDISSetDefValBaseClassImpl(const char*)

    bool		isSet() const			{ return isset_; }
    void		setSet( bool yesno=true )	{ isset_ = yesno; }

protected:

    uiString		truetext_;
    uiString		falsetext_;
    bool		yn_;
    bool		defaultyn_;
    bool		isset_;

};




/*! \brief Specifications for list of character string inputs.
*/
mExpClass(General) StringListInpSpec : public DataInpSpec
{ mODTextTranslationClass(StringListInpSpec);
public:
			StringListInpSpec(const BufferStringSet&);
			StringListInpSpec(const uiStringSet&);
			StringListInpSpec(const char** sl=0);
			StringListInpSpec(const uiString*);
			/*!<Similar to const char** Adds strings until an empty
			 string is found. */
			StringListInpSpec(const EnumDef&);
			//!<EnumDef is assumed to remain in mem
			StringListInpSpec(const StringListInpSpec&);
			~StringListInpSpec();

    virtual bool	isUndef(int idx=0) const;

    virtual DataInpSpec* clone() const;

    const uiStringSet& strings() const;

    const EnumDef*	enumDef() const		{ return enumdef_ ; }
    void		setEnumDef(const EnumDef&);
			//!<EnumDef is assumed to remain in mem

    void		addString(const uiString& txt);
    virtual const char*	text(int idx=0) const;
    void		setItemText(int idx, const uiString&);
    virtual bool	setText(const char* s,int nr);

    virtual int		getIntValue(int idx=0) const;
    virtual od_int64	getInt64Value(int idx=0) const;
    virtual double	getDValue(int idx=0) const;
    virtual float	getFValue(int idx=0) const;

    virtual void	setValue(int i,int idx=0);
    virtual void	setValue(od_int64 i,int idx=0);
    virtual void	setValue(double d,int idx=0);
    virtual void	setValue(float f,int idx=0);
			mDefDISSetValBaseClassImpl(bool)

    virtual void	setDefaultValue(int i,int idx=0);
    virtual int		getDefaultIntValue(int idx=0) const;
			mDefDISSetDefValBaseClassImpl(const char*)
			mDefDISSetDefValBaseClassImpl(float)
			mDefDISSetDefValBaseClassImpl(double)
			mDefDISSetDefValBaseClassImpl(bool)

    bool		isSet() const			{ return isset_; }
    void		setSet( bool yn=true )		{ isset_ = yn; }

protected:

    uiStringSet		strings_;
    int			cur_;
    int			defaultval_;
    bool		isset_;
    const EnumDef*	enumdef_;
};


/*! \brief Specifications for BinID/Coordinate/TrcNrs and offsets */

mExpClass(General) PositionInpSpec : public DataInpSpec
{ mODTextTranslationClass(PositionInpSpec);
public:

    struct Setup
    {
			Setup( bool wc=false, bool d2=false, bool ps=false )
			    : wantcoords_(wc)
			    , is2d_(d2)
			    , isps_(ps)
			    , isrel_(false)	{ clear(); }

	mDefSetupMemb(bool,wantcoords)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,isps)
	mDefSetupMemb(bool,isrel)
	mDefSetupMemb(Coord,coord)
	mDefSetupMemb(BinID,binid) // For 2D, put trace number in crl
	mDefSetupMemb(float,offs)

	void		clear()
			{
			    coord_.x_ = coord_.y_ = mUdf(double);
			    binid_.inl() = binid_.crl() = mUdf(int);
			    offs_ = 0;
			}
    };

			PositionInpSpec(const Setup&);
			PositionInpSpec(const BinID&,bool isps=false);
			PositionInpSpec(const Coord&,bool isps=false,
						     bool is2d=false);
			PositionInpSpec(int trcnr,bool isps=false);

    virtual DataInpSpec* clone() const	{ return new PositionInpSpec(*this); }
    virtual int	nElems() const;

    float		value( int idx=0 ) const
			{ return getVal(setup_,idx); }
    void		setValue( float f, int idx=0 )
			{ setVal( setup_, idx, f ); }
			mDefDISSetValBaseClassImpl(int)
			mDefDISSetValBaseClassImpl(od_int64)
			mDefDISSetValBaseClassImpl(bool)
			mDefDISSetValBaseClassImpl(double)
    virtual bool	isUndef(int idx=0) const;
    virtual const char*	text(int idx=0) const;
    virtual bool	setText(const char* s,int idx=0);

    float		defaultValue( int idx=0 ) const
			{ return getVal(defsetup_,idx); }
    void		setDefaultValue( float f, int idx=0 )
			{ setVal( defsetup_, idx, f ); }
			mDefDISSetDefValBaseClassImpl(int)
			mDefDISSetDefValBaseClassImpl(const char*)
			mDefDISSetDefValBaseClassImpl(double)
			mDefDISSetDefValBaseClassImpl(bool)

    Setup&		setup( bool def=false )
			{ return def ? defsetup_ : setup_; }
    const Setup&	setup( bool def=false ) const
			{ return def ? defsetup_ : setup_; }

    Coord		getCoord(double udfval=mUdf(double)) const;
    BinID		getBinID(int udfval=mUdf(int)) const;
    int			getTrcNr(int udfval=mUdf(int)) const;
    float		getOffset(float udfval=mUdf(float)) const;

protected:

    Setup		setup_;
    Setup		defsetup_;

    float		getVal(const Setup&,int) const;
    void		setVal(Setup&,int,float);
};
