#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.38 2002-03-12 12:11:40 arend Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <bufstring.h>
#include <string2.h>
#include <sizepolspec.h>
#include <basictypes.h>

class BinID2Coord;

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
class DataInpSpec : public SzPolicySpec
{
public:


			DataInpSpec( DataType t )
			    : SzPolicySpec(), tp_(t), pfw_(-1) {}

			DataInpSpec( const DataInpSpec& o )
			    : SzPolicySpec(o), tp_(o.tp_), pfw_(o.pfw_) {}

    virtual		~DataInpSpec() {}

    DataType		type() const			{ return tp_; }

    virtual DataInpSpec* clone() const			=0;
    virtual int 	nElems() const			{ return 1; }

    virtual bool	isUndef( int idx=0 ) const	=0;

    DataInpSpec&	setHSzP( SzPolicy p )		{ hor_=p; return *this;}
    DataInpSpec&	setVSzP( SzPolicy p )		{ ver_=p; return *this;}

			//! Preferred field width in characters
    DataInpSpec&	forcePrefFldWidth( int w )	{ pfw_=w; return *this;}
    int			prefFldWidth() const		{ return pfw_; }

    virtual bool	hasLimits() const		{ return false; }

    virtual const char*	text( int idx=0 ) const		=0;
    virtual void	setText( const char*, int idx=0)=0;

    virtual int		getIntValue( int idx=0 ) const
			    { return text(idx) ? atoi( text(idx) ) : 0; }
    virtual double	getValue( int idx=0 ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual float	getfValue( int idx=0 ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual bool	getBoolValue( int idx=0 ) const
			    { return (bool)getIntValue(idx); }

    virtual void	setValue( int i, int idx=0 )
			    { setText( toString( i ),idx); }
    virtual void	setValue( double d, int idx=0 )
			    { setText( toString( d ),idx); }
    virtual void	setValue( float f, int idx=0 )
			    { setText( toString( f ),idx); }
    virtual void	setValue( bool b, int idx=0 )
			    { setValue( ((int)b), idx ); }


protected:

    void		setType( DataType t )		{ tp_ = t; }
    DataType		tp_;
    int			pfw_;

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
			    : DataInpSpec( DataTypeImpl<T>() ) , limits_(0)
			    { value_ = undefVal<T>(); }
			NumInpSpec( T val ) 
			    : DataInpSpec( DataTypeImpl<T>() ) , limits_(0)
			    , value_( val )			{}
			NumInpSpec( const NumInpSpec<T>& o )
			    : DataInpSpec( o ) 
			    , limits_( o.limits_?new Interval<T>(*o.limits_):0 )
			    , value_( o.value_ )		{}

    virtual NumInpSpec<T>* clone() const
			    { return new NumInpSpec<T>( *this ); }

    virtual bool	isUndef( int idx=0 ) const	
			    { return isUndefined(value_); }

    virtual void	setText( const char* s, int idx=0 )
			    { getFromString( value_, s ); }

    T			value() const	
			{
			    if ( isUndefined(value_) ) return undefVal<T>();
			    return value_;
			}

    virtual const char*	text( int idx=0 ) const
			{
			    if ( isUndef() )	return "";
			    else		return toString( value() );
			}

    virtual bool	hasLimits() const		{ return limits_; }
    const Interval<T>*	limits()			{ return limits_; }
    NumInpSpec<T>&	setLimits( const Interval<T>& r)
			{
			    delete limits_;
			    limits_ = new Interval<T>( r );
			    return *this;
			}

protected:

    T			value_;

    Interval<T>*	limits_;
}; 


typedef NumInpSpec<int>		IntInpSpec;
typedef NumInpSpec<float>	FloatInpSpec;
typedef NumInpSpec<int>		DoubleInpSpec;


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
							undefVal<T>(), 
							undefVal<T>(), 
							undefVal<T>() ) 
						  : new Interval<T>(
							undefVal<T>(), 
							undefVal<T>() ) )
			    {}

			NumInpIntervalSpec( const Interval<T>& interval ) 
			    : DataInpSpec( DataTypeImpl<T>(DataType::interval) )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
			    , interval_( interval.clone() )	{}

			NumInpIntervalSpec( const NumInpIntervalSpec<T>& o )
			    : DataInpSpec( o )
			    , startlimits_(0), stoplimits_(0), steplimits_(0)
			    , interval_( o.interval_ ? o.interval_->clone() : 0)
			    {}

			~NumInpIntervalSpec()	
			    { 
				delete interval_; 
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
				return isUndefined( value_(idx) ); 
			    }

    virtual void	setValue( const Interval<T>& intval )
			    {
				if ( interval_ ) delete interval_;
				interval_ = intval.clone();
			    }

    virtual void	setText( const char* s, int idx=0 )
			    { 
				if ( pt_value_(idx) ) 
				    getFromString( *pt_value_(idx), s ); 
			    }

    T			value( int idx=0 ) const
			    {
				if ( isUndef(idx) ) return undefVal<T>();
				return value_(idx);
			    }

    virtual const char*	text( int idx=0 ) const
			    {
				if ( isUndef(idx) )	return "";
				return toString( value(idx) );
			    }

    virtual bool	hasLimits() const	
			    { return startlimits_||stoplimits_||steplimits_; }


			/*! \brief gets limits for interval components.

			    idx =  0: returns start limits
			    idx =  1: returns stop limits
			    idx =  2: returns step limits
			*/
    const Interval<T>*	limits( int idx=0 )
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

    Interval<T>*	startlimits_;
    Interval<T>*	stoplimits_;
    Interval<T>*	steplimits_;

    T			value_( int idx=0 ) const
			{
			    if ( pt_value_(idx) ) return *pt_value_(idx);
			    return undefVal<T>();
			}

    T*			pt_value_( int idx=0 ) const
			{
			    if ( !interval_ )	return 0;
			    if ( idx == 0 )	return &interval_->start;
			    if ( idx == 1 )	return &interval_->stop;
			    if ( hasStep() )	return &stpi()->step; 
			    return 0;
			}

    StepInterval<T>*	stpi() const
			{
			    mDynamicCastGet(const StepInterval<T>*,si,interval_)
			    return const_cast<StepInterval<T>*>( si );
			}
}; 


typedef NumInpIntervalSpec<int>		IntInpIntervalSpec;
typedef NumInpIntervalSpec<float>	FloatInpIntervalSpec;
typedef NumInpIntervalSpec<int>		DoubleInpIntervalSpec;


/*! \brief Specifications for character string inputs. */
class StringInpSpec : public DataInpSpec
{
public:
			StringInpSpec( const char* s=0 )
			    : DataInpSpec( DataTypeImpl<const char*>() )
			    , isUndef_(s?false:true), str( s ) 
			    { setHSzP(SzPolicySpec::medvar); }

    virtual bool	isUndef( int idx=0 ) const	{ return isUndef_; }

    virtual DataInpSpec* clone() const	{ return new StringInpSpec( *this ); }
    const char*		text() const			{ return str; }

    virtual void	setText( const char* s, int idx=0 ) 
			    { str = s; isUndef_ = s ? false : true; }
    virtual const char*	text( int idx=0 ) const
			    {
				if ( isUndef() )	return "";
				else		return (const char*) str;
			    }
protected:

    bool		isUndef_;
    BufferString	str;

};

/*! \brief Specifications for file-name inputs.
*/
class FileNameInpSpec : public StringInpSpec
{
public:
			FileNameInpSpec( const char* fname=0 )
			    : StringInpSpec( fname )
			    { 
				setType( DataTypeImpl<const char*>
						    ( DataType::filename ) );
			    }

    virtual DataInpSpec* clone() const  
			    { return new FileNameInpSpec( *this ); }
};


/*! \brief Specifications for boolean inputs.

This specifies a boolean input field. If the second char string is "" or NULL, 
then a checkbox will be created. Otherwise two connected radio buttons
will do the job. Default will create two radio buttons "Yes" and "No".
When calling setText("xx") on the resulting uiGenInput, it will try to set the
boolean value according to the given true/false text's or "Yes"/"No".
It does not change the underlying true/false texts.

*/

class BoolInpSpec : public DataInpSpec
{
public:
			BoolInpSpec( const char* truetxt="Yes"
				, const char* falsetxt="No" , bool yesno=true )
			    : DataInpSpec( DataTypeImpl<bool>() )
			    , truetext( truetxt ), falsetext( falsetxt )
			    , yn( yesno ) 
			    { setHSzP( SzPolicySpec::smallvar ); }

			BoolInpSpec( const BoolInpSpec& oth)
			    : DataInpSpec( oth )
			    , truetext( oth.truetext )
			    , falsetext( oth.falsetext )
			    , yn( oth.yn ) 
			    {}

    virtual bool	isUndef( int idx=0 ) const	{ return false; }

    virtual DataInpSpec* clone() const  
			    { return new BoolInpSpec( *this ); }
    const char*		trueFalseTxt( bool tf = true ) const
			    { return tf ? truetext : falsetext; }
    void 		setTrueFalseTxt( bool tf, const char* txt )
			    { if ( tf ) truetext=txt; else falsetext=txt; }

    bool		checked() const			{ return yn; }
    void		setChecked( bool yesno )	{ yn=yesno; }
    virtual const char*	text( int idx=0 ) const
			    { 
				return yn ? (const char*)truetext 
					  : (const char*)falsetext; 
			    }

    virtual void	setText( const char* s, int idx=0 )
			    { yn = s && strcmp(s,falsetext); }
    virtual bool	getBoolValue( int idx=0 ) const
			    { return yn; }
    virtual void	setValue( bool b, int idx=0 )
			    { yn = b; }

protected:

    BufferString	truetext;
    BufferString	falsetext;
    bool		yn;

};




/*! \brief Specifications for list of character string inputs.
*/
class StringListInpSpec : public DataInpSpec
{
public:
			StringListInpSpec( const char** sl=0 )
			    : DataInpSpec( DataTypeImpl<const char*>
						    (DataType::list) )
			    , cur_(0)
			    {
				setHSzP(SzPolicySpec::medvar); 

				if ( !sl ) return;
				while( *sl )
				    strings_ += new BufferString( *sl++ );
			    }

			StringListInpSpec( const StringListInpSpec& oth)
			    : DataInpSpec( oth )
			    , cur_(oth.cur_)
			    { deepCopy( strings_, oth.strings_ ); }

			~StringListInpSpec()
			    { deepErase(strings_); }

    virtual bool	isUndef( int idx=0 ) const	
			    { return !(strings_.size() && cur_ >= 0); }

    virtual DataInpSpec* clone() const	
			    { return new StringListInpSpec( *this ); }

    const ObjectSet<BufferString>& strings() const
			    { return strings_; }

    void		addString( const char* txt )
			    { strings_ += new BufferString( txt ); }

    virtual const char*	text( int idx=0 ) const
			    {
				if ( isUndef() ) return "";
				else return (const char*)*strings_[cur_];
			    }

    void		setItemText( int idx, const char* s ) 
			    { *strings_[cur_] = s; }

    virtual void	setText( const char* s, int nr ) 
			    {
				for ( int idx=0; idx<strings_.size(); idx++ )
				    if ( *strings_[idx] == s ) 
					{ cur_ = idx; return; }
			    }

    virtual int		getIntValue( int idx=0 ) const
			    { return cur_; }
    virtual double	getValue( int idx=0 ) const
			    { return cur_; }
    virtual float	getfValue( int idx=0 ) const
			    { return cur_; }

    virtual void	setValue( int i, int idx=0 )
			    { if ( i < strings_.size() ) cur_ = i; }
    virtual void	setValue( double d, int idx=0 )
			    { 
				if ( (int)(d+.5) < strings_.size() )
				    cur_ = (int)(d+.5); 
			    }
    virtual void	setValue( float f, int idx=0 )
			    { 
				if ( (int)(f+.5) < strings_.size() )
				    cur_ = (int)(f+.5); 
			    }

protected:

    ObjectSet<BufferString> strings_;
    int			cur_;

};


/*! \brief Specifications for BinID/Coordinate inputs.
*/
class BinIDCoordInpSpec : public DataInpSpec
{
public:
			BinIDCoordInpSpec( bool doCoord=false
					 , bool isRelative=false
					 , double inline_x = undefVal<double>()
					 , double crossline_y =
							     undefVal<double>()
					 , bool withOtherBut=false
					 , const BinID2Coord* b2c=0 )
			    : DataInpSpec( DataTypeImpl<int>(DataType::binID) )
			    , withOtherBut_( withOtherBut )
			    , inl_x( inline_x )
			    , crl_y( crossline_y )
			    , doCoord_( doCoord )
			    , isRelative_( isRelative )
			    , b2c_( b2c ) {}

    virtual DataInpSpec* clone() const  
			    { return new BinIDCoordInpSpec( *this ); }

    virtual int 	nElems()  const	{ return 2; }

    double		value( int idx=0 ) const { return idx ? crl_y : inl_x; }
    virtual bool	isUndef( int idx=0 ) const		
			    { 
				if ( idx<0 || idx>1 ) return true;
				return idx ? isUndefined( crl_y )
					   : isUndefined( inl_x );
			    }
    virtual const char*	text( int idx=0 ) const
			    {
				if ( isUndef() )	return "";
				else		return toString( value(idx) );
			    }
    virtual void	setText( const char* s, int idx=0 ) 
			    { getFromString( (idx ? crl_y : inl_x), s); }

    const char*		otherTxt() const
			    {
				if ( !withOtherBut_ ) return 0;
				if ( doCoord_ ) { return "Inline/Xline ..."; }
				return isRelative_? "Distance ...":"Coords ...";
			    }
    const BinID2Coord*	binID2Coord() const
			    { return b2c_; }

protected:

    double		inl_x;
    double		crl_y;

    bool		doCoord_;
    bool		isRelative_;
    bool		withOtherBut_;
    const BinID2Coord*	b2c_;
};

#endif
