#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.14 2001-05-10 07:29:36 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <bufstring.h>
class BinID2Coord;

#define mPrefNumWdt	10
#define mUndefWdt	-1	

/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/
class DataInpSpec
{
public:

    enum		Type { none, intTp, floatTp, doubleTp, boolTp, 
			       intIntervalTp, floatIntervalTp, doubleIntervalTp,
			       binIDCoordTp, stringTp, fileNmTp, stringListTp };

			DataInpSpec( Type t , int prefFldWdt )
			    : tp_(t), pfw_(prefFldWdt) {}

    virtual		~DataInpSpec() {}

    Type		type() const			{ return tp_; }

    virtual DataInpSpec* clone() const			=0;
    virtual int 	nElems() const			{ return 1; }
    virtual void	getText( BufferString&, int idx=0 ) const =0;

    virtual bool	isUndef() const			=0;

    void		setPrefFldWidth( int w )	{ pfw_=w; }
    int			prefFldWidth() const		{ return pfw_; }

protected:

    void		setType( Type t ) { tp_ = t; }
    Type		tp_;
    int			pfw_;

};



/*! \brief Specifications for inputs that optionally lie a specified range
*/
template <class T>
class NumInpWithLimitsSpec : public DataInpSpec
{
public:
			NumInpWithLimitsSpec(DataInpSpec::Type t ) 
			    : DataInpSpec( t, mPrefNumWdt ), limits_(0) {}
			NumInpWithLimitsSpec( const NumInpWithLimitsSpec<T>& o )
			    : DataInpSpec( o ), limits_(0) {}
			~NumInpWithLimitsSpec()	{ delete limits_; }

    bool		hasLimits() const	{ return limits_?true:false; }
    const Interval<T>*	limits()		{ return limits_; }
    void		setLimits( const Interval<T>& r)
			{
			    if( limits_ ) delete limits_;
			    limits_ = new Interval<T>( r );
			}

protected:

    Interval<T>*	limits_;

};


/*! \brief Specifications for numerical inputs
*/
template <class T>
class NumInpSpec : public NumInpWithLimitsSpec<T>
{
public:
			NumInpSpec(DataInpSpec::Type t ) 
			    : NumInpWithLimitsSpec<T>( t ), isUndef_( true ) {}
			NumInpSpec(DataInpSpec::Type t, T val ) 
			    : NumInpWithLimitsSpec<T>( t )
			    , isUndef_( false ), value_( val ) {}
			NumInpSpec( const NumInpSpec<T>& o )
			    : NumInpWithLimitsSpec<T>( o )
			    , isUndef_( o.isUndef_ )
			    , value_( o.value_ ) {}

    virtual bool	isUndef() const	{ return isUndef_; }
    T			value() const	
			{
			    if ( !isUndef() ) return value_;

			    switch( type() )
			    {
				case floatTp: case doubleTp:
				    return (T)mUndefValue;
				default:
				    return (T)0;
			    }
			}

    virtual void	getText( BufferString& dest, int idx ) const
			{
			    if ( isUndef() )	dest = "";
			    else		dest = value();
			}

protected:

    bool		isUndef_;
    T			value_;

}; 


#define mDefNumInpClass( clssNm, type ) \
class  clssNm : public NumInpSpec<type>\
{\
public:\
				clssNm() \
				    : NumInpSpec<type>( type##Tp )	{} \
				clssNm( type var ) \
				    : NumInpSpec<type>( type##Tp, var )	{} \
				clssNm( const clssNm& o ) \
				    : NumInpSpec<type>(o)		{} \
\
    virtual DataInpSpec*	clone() const \
				{ return new clssNm( *this ); } \
};

/*! \brief Specifications for float inputs.  */
mDefNumInpClass( FloatInpSpec, float )

/*! \brief Specifications for double inputs.  */
mDefNumInpClass( DoubleInpSpec, double )

/*! \brief Specifications for integer inputs.  */
mDefNumInpClass( IntInpSpec, int )

#undef mDefNumInpClass

/*! \brief Specifications for numerical intervals
*/
template <class T>
class NumInpIntervalSpec : public NumInpWithLimitsSpec<T>
{
public:
			NumInpIntervalSpec(DataInpSpec::Type t,
			    Interval<T>* interval=0 ) 
			    : NumInpWithLimitsSpec<T>( t )
			    , interval_( interval ) {}
			NumInpIntervalSpec( const NumInpIntervalSpec<T>& o )
			    : NumInpWithLimitsSpec<T>( o )
			    , interval_( o.interval_ ){}
			~NumInpIntervalSpec()	{ delete interval_; }

    virtual bool	isUndef() const { return interval_ ? false : true; }
    virtual int 	nElems() const	{ return hasStep() ? 3 : 2; }
    bool		hasStep() const	{ return stpi() ? true : false; }

    T			value( int idx=0 ) const
			{
			    if( isUndef() )
			    {
				switch( type() )
				{
				    case floatIntervalTp: case doubleIntervalTp:
					return (T) mUndefValue;
				    default:
					 return (T)0;
				}
			    }

			    if ( idx == 0 )	return interval_->start;
			    if ( idx == 1 )	return interval_->stop;
			    if ( hasStep() )	return stpi()->step; 
			    return (T)0;
			}

    virtual void	getText( BufferString& dest, int idx ) const
			{
			    if ( isUndef() )	dest = "";
			    else		dest = value(idx);
			}

protected:

    StepInterval<T>*	stpi() const
			{ return dynamic_cast< StepInterval<T>* > (interval_);}

    Interval<T>*	interval_;

}; 


#define mDefIntervalClass( clssNm, type ) \
class  clssNm : public NumInpIntervalSpec<type> \
{ \
public: \
		    clssNm( Interval<type>* var=0 ) \
		    : NumInpIntervalSpec<type>( type##IntervalTp, var ) {} \
		    clssNm( const clssNm& o ) \
		    : NumInpIntervalSpec<type>(o)	{} \
\
    virtual DataInpSpec*	clone() const \
				{ return new clssNm( *this ); } \
};

/*! \brief Specifications for float inputs.  */
mDefIntervalClass( FloatInpIntervalSpec, float )

/*! \brief Specifications for double inputs.  */
mDefIntervalClass( DoubleInpIntervalSpec, double )

/*! \brief Specifications for integer inputs.  */
mDefIntervalClass( IntInpIntervalSpec, int )

#undef mDefIntervalClass

/*! \brief Specifications for character string inputs. */

class StringInpSpec : public DataInpSpec
{
public:
			StringInpSpec( const char* s=0 )
			    : DataInpSpec( stringTp, mUndefWdt )
			    , isUndef_(s?false:true), str( s ) {}

    virtual bool	isUndef() const			{ return isUndef_; }

    virtual DataInpSpec* clone() const	{ return new StringInpSpec( *this ); }
    const char*		text() const			{ return str; }
    void		setText( const char* txt )	{ str = txt; }

    virtual void	getText( BufferString& dest, int idx ) const
			{
			    if( isUndef() )	dest = "";
			    else		dest = str;
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
			    { setType( fileNmTp ); }

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
			    : DataInpSpec( boolTp, mUndefWdt )
			    , truetext( truetxt ), falsetext( falsetxt )
			    , yn( yesno ) {}

    virtual bool	isUndef() const			{ return false; }

    virtual DataInpSpec* clone() const  
			    { return new BoolInpSpec( *this ); }
    const char*		trueFalseTxt( bool tf = true ) const
			    { return tf ? truetext : falsetext; }
    void 		setTrueFalseTxt( bool tf, const char* txt )
			    { if( tf ) truetext=txt; else falsetext=txt; }

    bool		checked() const			{ return yn; }
    void		setChecked( bool yesno )	{ yn=yesno; }
    virtual void	getText( BufferString& dest, int idx ) const
			    { dest = yn ? truetext : falsetext; }

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
				    : DataInpSpec( stringListTp, mUndefWdt )
				{
				    if ( !sl ) return;
				    while( *sl )
					strings_ += new BufferString( *sl++ );
				}

				StringListInpSpec( const StringListInpSpec& oth)
				    : DataInpSpec( stringListTp, mUndefWdt )
				    { deepCopy( strings_, oth.strings_ ); }

				~StringListInpSpec() { deepErase(strings_); }

    virtual bool		isUndef() const	
				    { return strings_.size()?false:true; }
    virtual DataInpSpec*	clone() const	
				    { return new StringListInpSpec( *this ); }
    const ObjectSet<BufferString>& strings() const	{ return strings_; }
    void			addString( const char* txt )
				    { strings_ += new BufferString( txt ); }
    virtual void		getText( BufferString& dest, int idx ) const
				{
				    if( isUndef() )	dest = "";
				    else		dest = *strings_[idx];
				}

protected:

    ObjectSet<BufferString>	strings_;

};


/*! \brief Specifications for BinID/Coordinate inputs.
*/
class BinIDCoordInpSpec : public DataInpSpec
{
public:
			BinIDCoordInpSpec( bool doCoord=false
					 , bool isRelative=false
					 , double inline_x = mUndefValue
					 , double crossline_y = mUndefValue
					 , bool withOtherBut=true
					 , const BinID2Coord* b2c=0 )
			    : DataInpSpec( binIDCoordTp, mPrefNumWdt )
			    , withOtherBut_( withOtherBut )
			    , inl_x( inline_x )
			    , crl_y( crossline_y )
			    , doCoord_( doCoord )
			    , isRelative_( isRelative )
			    , b2c_( b2c ) {}

    virtual DataInpSpec* clone() const  
			{ return new BinIDCoordInpSpec( *this ); }
    double		value( int idx ) const { return idx ? crl_y : inl_x; }
    virtual bool	isUndef() const		
			{ return mIsUndefined(inl_x) && mIsUndefined(crl_y); }
    virtual void	getText( BufferString& dest, int idx ) const
			{
			    if( isUndef() )	dest = "";
			    else		dest = value( idx );
			}
    const char*		otherTxt() const
			{
			    if( !withOtherBut_ ) return 0;
			    if( doCoord_ ) { return "Inline/Xline ..."; }
			    return isRelative_? "Distance ..." : "Coords ...";
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
