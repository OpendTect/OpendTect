#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.1 2001-05-01 09:29:29 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <bufstring.h>

/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/

class DataInpSpec
{
public:

    enum		Type { None, intTp, floatTp, doubleTp, boolTp, 
			       stringTp, fileNmTp };

			DataInpSpec( Type t = None) { tp_ = t; }
    Type		type() const { return tp_; }

    virtual DataInpSpec* clone() const =0;
    virtual void	getText( BufferString& ) const =0;

protected:

    void		setType( Type t ) { tp_ = t; }
    Type		tp_;

};


/*! \brief Specifications for numerical inputs
*/
template <class T>
class NumInp : public DataInpSpec
{
public:
			NumInp(DataInpSpec::Type t, T val ) 
			    : DataInpSpec( t ), rng(0), value_( val ) {}

			NumInp( const NumInp<T>& o )
			    : DataInpSpec( o ), rng(0), value_( o.value_ )
			    { if( o.rng ) setRange( *o.rng ); }

    const Interval<T>*	range() { return rng; }
    void		setRange( const Interval<T>& r)
			    {
				if( rng ) delete rng;
				rng = new Interval<T>( r );
			    }
    T			value() const { return value_; }

    virtual void	getText( BufferString& dest) const
			    { dest = value(); }

protected:

    Interval<T>*	rng;
    T			value_;

}; 


#define mDefNumInpClass( clssNm, type ) \
    class  clssNm : public NumInp<type>\
    {\
    public:\
			clssNm( type var=0 ) : NumInp<type>( type##Tp, var ) {}\
			clssNm( const clssNm& o ) \
			: NumInp<type>(o)	{} \
     \
    virtual clssNm*	clone() const \
			{ return new clssNm( *this ); } \
    };

/*! \brief Specifications for float inputs.  */
mDefNumInpClass( FloatInp, float )

/*! \brief Specifications for double inputs.  */
mDefNumInpClass( DoubleInp, double )

/*! \brief Specifications for integer inputs.  */
mDefNumInpClass( IntInp, int )


/*! \brief Specifications for character string inputs.
*/
class StringInp : public DataInpSpec
{
public:
			StringInp( const char* s=0, int prefWdt=-1 );

    virtual DataInpSpec* clone() const	{ return new StringInp( *this ); }
    const char*		text() const	{ return str; }
    void		setText( const char* txt) { str = txt; }
    int			prefWidth() const { return pw; }

    virtual void	getText( BufferString& dest) const
			    { dest = str; }
protected:

    BufferString	str;
    int			pw;
};

/*! \brief Specifications for file-name inputs.
*/
class FileNameInp : public StringInp
{
public:
			FileNameInp( const char* fname=0, int prefWdt=-1 );

    virtual DataInpSpec* clone() const  
			    { return new FileNameInp( *this ); }
};


/*! \brief Specifications for boolean inputs.

This specifies a boolean input field. If the second char string is "" or NULL, 
then a checkbox will be created. Otherwise two connected radio buttons
will do the job. Default will create two radio buttons "Yes" and "No".
When calling setText("xx") on the resulting uiGenInput, it will try to set the
boolean value according to the given true/false text's or "Yes"/"No".
It does not change the underlying true/false texts.

*/

class BoolInp : public DataInpSpec
{
public:
			BoolInp( const char* truetxt="Yes"
				, const char* falsetxt="No" , bool yesno=true )
			    : DataInpSpec( boolTp )
			    , truetext( truetxt ), falsetext( falsetxt )
			    , yn( yesno ) {}

    virtual DataInpSpec* clone() const  
			    { return new BoolInp( *this ); }

    const char*		trueFalseTxt( bool tf = true ) const
			    { return tf ? truetext : falsetext; }
    void 		setTrueFalseTxt( bool tf, const char* txt )
			    { if( tf ) truetext=txt; else falsetext=txt; }

    bool		checked() const			{ return yn; }
    void		setChecked( bool yesno )	{ yn=yesno; }

    virtual void	getText( BufferString& dest) const
			    { dest = yn ? truetext : falsetext; }

protected:

    BufferString	truetext;
    BufferString	falsetext;
    bool		yn;
};

#endif
