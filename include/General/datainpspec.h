#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.2 2001-05-01 15:37:10 arend Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <bufstring.h>
#include <survinfo.h>

/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/
class DataInpSpec
{
public:

    enum		Type { None, intTp, floatTp, doubleTp, boolTp, 
			       intIntervalTp, floatIntervalTp, doubleIntervalTp,
			       binIdCoordTp, stringTp, fileNmTp, stringListTp };

			DataInpSpec( Type t = None) { tp_ = t; }

    virtual		~DataInpSpec() {}

    Type		type() const { return tp_; }

    virtual DataInpSpec* clone() const =0;
    virtual int 	nElems()				{ return 1; }
    virtual void	getText( BufferString&, int idx=0 ) const =0;

protected:

    void		setType( Type t ) { tp_ = t; }
    Type		tp_;

};



/*! \brief Specifications for inputs that optionally lie a specified range
*/
template <class T>
class NumInpWithLimitsSpec : public DataInpSpec
{
public:
			NumInpWithLimitsSpec(DataInpSpec::Type t ) 
			    : DataInpSpec( t ), limits_(0) {}

			NumInpWithLimitsSpec( const NumInpWithLimitsSpec<T>& o )
			    : DataInpSpec( o ), limits_(0) {}

			~NumInpWithLimitsSpec()		{ delete limits_; }

    const Interval<T>*	limits()			{ return limits_; }
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
			NumInpSpec(DataInpSpec::Type t, T val ) 
			    : NumInpWithLimitsSpec<T>( t ), value_( val ) {}

			NumInpSpec( const NumInpSpec<T>& o )
			    : NumInpWithLimitsSpec<T>( o )
			    , value_( o.value_ ) {}

    T			value() const { return value_; }

    virtual void	getText( BufferString& dest, int idx ) const
			    { dest = value(); }

protected:

    T			value_;
}; 


#define mDefNumInpClass( clssNm, type ) \
    class  clssNm : public NumInpSpec<type>\
    {\
    public:\
			clssNm( type var=0 ) \
			    : NumInpSpec<type>( type##Tp, var )	{}\
			clssNm( const clssNm& o ) \
			    : NumInpSpec<type>(o)		{} \
     \
    virtual clssNm*	clone() const \
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

    virtual int 	nElems()	{ return hasStep() ? 3 : 2; }
    bool		hasStep()	{ return stpi() ? TRUE : FALSE; }

    virtual void	getText( BufferString& dest, int idx ) const
			{
			    if( !idx )
				{ dest = interval_.start ; return; }
			    if( idx == 1 )
				{ dest = interval_.start ; return; }

			    if( hasStep() ) dest = stpi()->step; 
			}

protected:

    Interval<T>*	stpi()
			{ return dynamic_cast< StepInterval<T>* > (interval_);}

    Interval<T>*	interval_;

}; 


#define mDefIntervalClass( clssNm, type ) \
    class  clssNm : public NumInpIntervalSpec<type>\
    {\
    public:\
			clssNm( Interval<type>* var=0 ) \
			: NumInpIntervalSpec<type>( type##IntervalTp, var ) {}\
			clssNm( const clssNm& o ) \
			: NumInpIntervalSpec<type>(o)	{} \
     \
    virtual clssNm*	clone() const \
			{ return new clssNm( *this ); } \
    };

/*! \brief Specifications for float inputs.  */
mDefIntervalClass( FloatInpIntervalSpec, float )

/*! \brief Specifications for double inputs.  */
mDefIntervalClass( DoubleInpIntervalSpec, double )

/*! \brief Specifications for integer inputs.  */
mDefIntervalClass( IntInpIntervalSpec, int )

#undef mDefIntervalClass

/*! \brief Specifications for character string inputs.
*/
class StringInpSpec : public DataInpSpec
{
public:
			StringInpSpec( const char* s=0, int prefWdt=-1 )
			    : DataInpSpec( stringTp ), str( s ), pw( prefWdt ){}

    virtual DataInpSpec* clone() const	{ return new StringInpSpec( *this ); }
    const char*		text() const	{ return str; }
    void		setText( const char* txt) { str = txt; }
    int			prefWidth() const { return pw; }

    virtual void	getText( BufferString& dest, int idx ) const
			    { dest = str; }
protected:

    BufferString	str;
    int			pw;
};

/*! \brief Specifications for file-name inputs.
*/
class FileNameInpSpec : public StringInpSpec
{
public:
			FileNameInpSpec( const char* fname=0, int prefWdt=-1 )
			    : StringInpSpec( fname, prefWdt )
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
			    : DataInpSpec( boolTp )
			    , truetext( truetxt ), falsetext( falsetxt )
			    , yn( yesno ) {}

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
class stringListInpSpec : public DataInpSpec
{
public:
				stringListInpSpec( const char** sl=0
					     , int prefWdt=-1 )
				    : DataInpSpec( stringTp )
				    ,pw( prefWdt )
				    { 
					const char** s=sl;
					while( *s++ )
					    strings_ += new BufferString(*s);
				    }

				~stringListInpSpec() { deepErase(strings_); }

    virtual DataInpSpec*	clone() const	
				    { return new stringListInpSpec( *this ); }

    const ObjectSet<BufferString>& strings() const	{ return strings_; }
    void			addString( const char* txt) 
				    { strings_ += new BufferString( txt); }

    int				prefWidth() const	{ return pw; }

    virtual void		getText( BufferString& dest, int idx ) const
				    { dest = *strings_[idx]; }
protected:

    ObjectSet<BufferString>	strings_;
    int				pw;
};


/*! \brief Specifications for BinId/Coordinate inputs.
*/
class binIdCoordInpSpec : public DataInpSpec
{
public:
			binIdCoordInpSpec( bool doCoord=false
					 , bool isRelative=false
					 , const SurveyInfo& si = SI() )
			    : DataInpSpec( binIdCoordTp )
			    , doCoord_( doCoord )
			    , isRelative_( isRelative )
			    , surv_( si ) {}

    virtual DataInpSpec* clone() const  
			    { return new binIdCoordInpSpec( *this ); }

    virtual void	getText( BufferString& dest, int idx ) const
			    {
				if( doCoord_ )
				    { dest = "Inline/Crossline"; return; }
				dest = isRelative_ ? "Distance" : "Coords";
			    }

    const SurveyInfo&	survInf()	{ return surv_;}

protected:

    bool		doCoord_;
    bool		isRelative_;
    const SurveyInfo&	surv_;
};

#endif
