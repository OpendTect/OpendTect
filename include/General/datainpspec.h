#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.66 2007-08-13 13:11:16 cvsjaap Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "string2.h"
#include "undefval.h"
#include "bufstringset.h"
#include "position.h"
#include "keystrs.h"

class RCol2Coord;
class IOPar;

class DataType
{
public:

    enum		Rep  { intTp, floatTp, doubleTp, boolTp, stringTp };
    enum		Form { normal, interval, filename, list, binID };

			DataType( Rep tp, Form frm=normal ) 
			    : rep_( tp ), form_(frm) {}

    inline Rep		rep() const		{ return rep_; }
    inline Form		form() const		{ return form_; }

    bool 		operator==( const DataType& oth ) const
			    { return (rep_==oth.rep_) && (form_==oth.form_); }
    bool       		operator!=( const DataType& oth ) const
			    { return ! (oth == *this); }


protected:

    Rep			rep_;
    Form		form_;

};


template<class T>
class DataTypeImpl : public DataType
{
public:


			DataTypeImpl( Form frm=normal ) 
			    : DataType( rep__( (T)0 ), frm ) {}
protected:

    inline Rep		rep__( int i )    const	{ return intTp; }
    inline Rep		rep__( float f )  const	{ return floatTp; }
    inline Rep		rep__( double d ) const	{ return doubleTp; }
    inline Rep		rep__( bool b )   const	{ return boolTp; }
    inline Rep		rep__( const char* s ) const	{ return stringTp; }

};




/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/
class DataInpSpec
{
public:


			DataInpSpec( DataType t );
			DataInpSpec( const DataInpSpec& o );
    virtual		~DataInpSpec() {}

    DataType		type() const;

    virtual DataInpSpec* clone() const			=0;
    virtual int 	nElems() const			{ return 1; }

    virtual bool	isUndef( int idx=0 ) const	=0;

    virtual bool	hasLimits() const		{ return false; }
    virtual bool	isInsideLimits(int idx=0) const;

    virtual const char*	text( int idx=0 ) const		=0;
    virtual bool	setText( const char*, int idx=0)=0;

    void		fillPar(IOPar&) const;
    			/*!\Saves the _values_ (from text()) */
    bool		usePar(const IOPar&);
    			/*!\Sets the _values_ (with setText()) */

    virtual int		getIntValue( int idx=0 ) const;
    virtual double	getdValue( int idx=0 ) const;
    virtual float	getfValue( int idx=0 ) const;
    virtual bool	getBoolValue( int idx=0 ) const;

    virtual void	setValue( int i, int idx=0 );
    virtual void	setValue( double d, int idx=0 );
    virtual void	setValue( float f, int idx=0 );
    virtual void	setValue( bool b, int idx=0 );

    virtual int		getDefaultIntValue( int idx=0 ) const;
    virtual double	getDefaultValue( int idx=0 ) const;
    virtual float	getDefaultfValue( int idx=0 ) const;
    virtual bool	getDefaultBoolValue( int idx=0 ) const;
    virtual const char*	getDefaultStringValue( int idx=0 ) const;

    virtual void	setDefaultValue( int i, int idx=0 )		{}
    virtual void	setDefaultValue( double d, int idx=0 )		{}
    virtual void	setDefaultValue( float f, int idx=0 )		{}
    virtual void	setDefaultValue( bool b, int idx=0 )		{}
    virtual void	setDefaultValue( const char* s, int idx=0 )	{}

    virtual const char*	name( int idx=0 ) const;
    virtual const 
	DataInpSpec&	setName( const char*, int idx=0);


protected:

    void		setType( DataType t );
    DataType		tp_;
    bool		prefempty_;
    
    TypeSet<int> 	nameidx_;
    TypeSet<const char*>   name_;

private:
    static const char*	valuestr;
};


/*! \brief Specifications for numerical inputs that may or may not have limits

    If the input must lie in a certain range, this range can be obtainted by
    Limits() and set by... setLimits().

*/
template <class T>
class NumInpSpec : public DataInpSpec
{
public:
			NumInpSpec() 
			    : DataInpSpec( DataTypeImpl<T>() )
			    , limits_(0)
			    { mSetUdf( value_ ); mSetUdf( defaultvalue_ ); }
			NumInpSpec( T val ) 
			    : DataInpSpec( DataTypeImpl<T>() )
			    , limits_(0)
			    , value_( val )
			    { mSetUdf( defaultvalue_ ); }
			NumInpSpec(const NumInpSpec<T>&);
			~NumInpSpec()			{ delete limits_; }

    virtual NumInpSpec<T>* clone() const
			    { return new NumInpSpec<T>( *this ); }

    virtual bool	isUndef( int idx=0 ) const	
			    { return mIsUdf(value_); }

    virtual bool	setText( const char* s, int idx=0 )
			    { return getFromString( value_, s ); }

    virtual int		getIntValue(int idx=0) const { return (int)value(); }
    virtual double	getdValue(int idx=0) const    { return value(); }
    virtual float	getfValue(int idx=0) const   { return value(); }

    virtual int		getDefaultIntValue(int idx=0) const
    			{ return (int)defaultValue(); }
    virtual double	getDefaultValue(int idx=0) const
    			{ return defaultValue(); }
    virtual float	getDefaultfValue(int idx=0) const
    			{ return defaultValue(); }
    
    virtual void	setDefaultValue( int val, int idx=0 )
			{ defaultvalue_ = (T)val; }
    virtual void	setDefaultValue( double val, int idx=0 )
			{ defaultvalue_ = (T)val; }
    virtual void	setDefaultValue( float val, int idx=0 )
			{ defaultvalue_ = (T)val; }
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
			    if ( isUndef(idx) ) return false;
			    if ( !limits_ ) return true;

			    return limits_->includes( value() );
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
typedef NumInpSpec<float>	FloatInpSpec;
typedef NumInpSpec<double>	DoubleInpSpec;


/*! \brief Specifications for numerical intervals

    Intervals consist of a start + stop value and optionally a step value, in
    which case the Interval is a StepInterval.

    For each of the interval components start, stop and step, separate
    limits can be set.

*/
template <class T>
class NumInpIntervalSpec : public DataInpSpec
{
public:
			NumInpIntervalSpec( bool withstep=false )
			    : DataInpSpec( DataTypeImpl<T>(DataType::interval) )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
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
			    : DataInpSpec( DataTypeImpl<T>(DataType::interval) )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
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
			    , interval_( o.interval_ ? o.interval_->clone() : 0)
			    , defaultinterval_( o.defaultinterval_ ? 
				    		o.defaultinterval_->clone() : 0)
			    {}

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

    virtual int 	nElems()  const	{ return hasStep() ? 3 : 2; }
    inline bool		hasStep() const	{ return stpi()    ? true : false; }

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

    virtual void	setDefaultValue( const Interval<T>& defaultintval )
			{
			    if ( defaultinterval_ ) delete defaultinterval_;
			    defaultinterval_ = defaultintval.clone();
			}

    virtual bool	setText( const char* s, int idx=0 )
			{ 
			    if ( pt_value_(idx) ) 
				return getFromString( *pt_value_(idx), s ); 
			    return false;
			}

    T			value( int idx=0 ) const
			{
			    if ( isUndef(idx) ) return mUdf(T);
			    return value_(idx);
			}

    virtual int		getIntValue(int idx=0) const { return (int)value(idx); }
    virtual double	getdValue(int idx=0) const    { return value(idx); }
    virtual float	getfValue(int idx=0) const   { return value(idx); }

    T			defaultValue( int idx=0 ) const
			{
			    if ( !defaultinterval_ ) return mUdf(T);
			    return defaultvalue_(idx);
			}

    virtual int		getDefaultIntValue(int idx=0) const
    			{ return (int)defaultValue(idx); }
    virtual double	getDefaultValue(int idx=0) const
    			{ return defaultValue(idx); }
    virtual float	getDefaultfValue(int idx=0) const
    			{ return defaultValue(idx); }
    
    virtual const char*	text( int idx=0 ) const
			{
			    if ( isUndef(idx) ) return "";
			    return toString( value(idx) );
			}

    virtual bool	hasLimits() const	
			{ return startlimits_||stoplimits_||steplimits_; }
    virtual bool	isInsideLimits(int idx=0) const
			{
			    if ( isUndef(idx) ) return false;
			    const Interval<T>* limits_ = limits(idx);
			    return limits_ ? limits_->includes( value(idx) ) 
					   : true;
			}


			/*! \brief gets limits for interval components.

			    idx =  0: returns start limits
			    idx =  1: returns stop limits
			    idx =  2: returns step limits
			*/
    const Interval<T>*	limits( int idx=0 ) const
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
    NumInpIntervalSpec&	setLimits( const Interval<T>& r, int idx=-1 )
			{
			    if ( idx < 0 )
			    {
				delete startlimits_; startlimits_ = 
						    new Interval<T>( r );
				delete stoplimits_; stoplimits_ =
						    new Interval<T>( r );
				if ( idx == -2 )
				{
				    delete steplimits_; steplimits_ =
						    new Interval<T>( r );
				}
				return *this;
			    }

			    switch ( idx )
			    {
			    case 0 : 
				delete startlimits_; 
				startlimits_ = new Interval<T>(r);
			    break;
			    case 1 :
				delete stoplimits_; 
				stoplimits_ = new Interval<T>(r);
			    break;
			    case 2 :
				delete steplimits_; 
				steplimits_ = new Interval<T>(r);
			    break;
			    }

			    return *this;
			}

protected:

    Interval<T>*	interval_;
    Interval<T>*	defaultinterval_;

    Interval<T>*	startlimits_;
    Interval<T>*	stoplimits_;
    Interval<T>*	steplimits_;

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
class StringInpSpec : public DataInpSpec
{
public:
			StringInpSpec( const char* s=0 );
    virtual bool	isUndef( int idx=0 ) const;

    virtual DataInpSpec* clone() const;
    const char*		text() const;

    virtual bool	setText( const char* s, int idx=0 ) ;
    virtual const char*	text( int idx ) const;

    void		setDefaultStringValue( const char* s, int idx );
    const char*		getDefaultStringValue( int idx ) const;
protected:

    bool		isUndef_;
    BufferString	str;
    BufferString	defaultstr;

};

/*! \brief Specifications for file-name inputs.
*/
class FileNameInpSpec : public StringInpSpec
{
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


class BoolInpSpec : public DataInpSpec
{
public:
			BoolInpSpec(bool yesno,const char* truetxt=sKey::Yes,
				    const char* falsetxt=sKey::No,
				    bool isset=true);
			BoolInpSpec(const BoolInpSpec&);

    virtual bool	isUndef(int idx=0) const;

    virtual DataInpSpec* clone() const;
    const char*		trueFalseTxt(bool tf=true) const;
    void 		setTrueFalseTxt(bool,const char*);

    bool		checked() const;
    void		setChecked(bool yesno);
    virtual const char*	text(int idx=0) const;

    virtual bool	setText(const char* s,int idx=0);
    virtual bool	getBoolValue(int idx=0) const;
    virtual void	setValue(bool,int idx=0);
    virtual bool	getDefaultBoolValue(int idx=0) const;
    virtual void	setDefaultValue(bool,int idx=0);

    bool		isSet() const 			{ return isset; }
    void		setSet( bool yesno=true )	{ isset = yesno; }

protected:

    BufferString	truetext;
    BufferString	falsetext;
    bool		yn;
    bool		defaultyn;
    bool		isset;

};




/*! \brief Specifications for list of character string inputs.
*/
class StringListInpSpec : public DataInpSpec
{
public:
    			StringListInpSpec( const BufferStringSet& bss );
			StringListInpSpec( const char** sl=0 );
			StringListInpSpec( const StringListInpSpec& oth );
			~StringListInpSpec();

    virtual bool	isUndef( int idx=0 ) const;

    virtual DataInpSpec* clone() const;

    const BufferStringSet& strings() const;

    void		addString( const char* txt );
    virtual const char*	text( int idx=0 ) const;
    void		setItemText( int idx, const char* s );
    virtual bool	setText( const char* s, int nr );

    virtual int		getIntValue( int idx=0 ) const;
    virtual double	getdValue( int idx=0 ) const;
    virtual float	getfValue( int idx=0 ) const;

    virtual void	setValue( int i, int idx=0 );
    virtual void	setValue( double d, int idx=0 );
    virtual void	setValue( float f, int idx=0 );

    virtual void	setDefaultValue( int i, int idx=0 );
    virtual int		getDefaultIntValue( int idx=0 ) const;

    bool		isSet() const			{ return isset_; }
    void		setSet( bool yn=true )		{ isset_ = yn; }
protected:

    BufferStringSet	strings_;
    int			cur_;
    int			defaultval_;
    bool		isset_;
};


/*! \brief Specifications for BinID/Coordinate inputs.
*/
class PositionInpSpec : public DataInpSpec
{
public:
			PositionInpSpec(bool docrd,
					float x_inl=mUdf(float),
					float y_crl=mUdf(float),
					bool isrel=false,
					const RCol2Coord* b2c=0);

    virtual DataInpSpec* clone() const;
    virtual int 	nElems()  const;

    float		value(int idx=0) const;
    virtual bool	isUndef(int idx=0) const;
    virtual const char*	text(int idx=0) const;
    virtual bool	setText(const char* s,int idx=0);

    const char*		otherTxt() const;
    const RCol2Coord*	binID2Coord() const;

    float		defaultValue(int idx=0) const;
    void		setDefaultValue(float f, int idx=0);
protected:

    double		x_inl_;
    double		y_crl_;
    double		defaultx_inl_;
    double		defaulty_crl_;

    bool		docoord_;
    bool		isrelative_;
    const RCol2Coord*	b2c_;
};


class BinIDInpSpec : public PositionInpSpec
{
public:
			BinIDInpSpec( const BinID& bid )
			    : PositionInpSpec(false,bid.inl,bid.crl)	{}
			BinIDInpSpec() : PositionInpSpec(false)		{}

    BinID		getValue() const
    			{ return BinID(mNINT(value(0)),mNINT(value(1))); }
    BinID		getDefaultValue() const
    			{
			    return BinID(mNINT(defaultValue(0)),
					 mNINT(defaultValue(1)));
			}
};


class CoordInpSpec : public PositionInpSpec
{
public:
			CoordInpSpec( const Coord& crd )
			    : PositionInpSpec(true,crd.x,crd.y)		{}
			CoordInpSpec() : PositionInpSpec(true)		{}

    Coord		getValue() const
    			{ return Coord(value(0),value(1)); }
    Coord		getDefaultValue() const
    			{
			    return Coord(mNINT(defaultValue(0)),
					 mNINT(defaultValue(1)));
			}
};

#endif
