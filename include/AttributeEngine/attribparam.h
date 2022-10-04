#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "attribparambase.h"
#include "datainpspec.h"


namespace Attrib
{

/*!
\brief A parameter that is used by an attribute.

  Each attribute has a definition string that defines how the attribute is
  computed. The definition string has the format:

  AttribNameWithoutSpaces param1=value1 param2=value2,value3

  The paramater thus has a key (e.g. param1) and one or more associated values.

  Subclasses are used to provide accurate parameter definition for parameters
  of each and every type.
*/

mExpClass(AttributeEngine) BinIDParam : public ValParam
{ mODTextTranslationClass(BinIDParam);
public:
				BinIDParam(const char*);
				BinIDParam(const char*,const BinID&,
					   bool isreq=true);
				~BinIDParam();

    BinIDParam*			clone() const override;
    void			setLimits(const Interval<int>& inlrg,
					  const Interval<int>& crlrg);
    void			setLimits(int mininl,int maxinl,
					  int mincrl,int maxcrl);

    bool			setCompositeValue(const char*) override;
    bool			getCompositeValue(BufferString&) const override;

    void			setDefaultValue(const BinID&);
    BinID			getDefaultBinIDValue() const;
    BufferString		getDefaultValue() const override;
    BinID			getValue() const;

    void			toString(BufferString&,const BinID&) const;
};


/*!
\brief Bool parameter that is used by an attribute.
*/

mExpClass(AttributeEngine) BoolParam : public ValParam
{ mODTextTranslationClass(BoolParam);
public:
				BoolParam(const char*);
				BoolParam(const char*,bool, bool isreq=true);
				~BoolParam();

    BoolParam*			clone() const override;

    bool			setCompositeValue(const char*) override;
    BufferString		getDefaultValue() const override;

    bool			isSet() const;
    void			setSet(bool yn=true);
};


/*!
\brief Enum parameter that is used by an attribute.
*/

mExpClass(AttributeEngine) EnumParam : public ValParam
{ mODTextTranslationClass(EnumParam);
public:
				EnumParam(const char*);
				EnumParam(const char*,int defval,
					  bool isreq=true);
				~EnumParam();

    EnumParam*			clone() const override;
    BufferString		getDefaultValue() const override;

    void			addEnum(const char*);
    void			addEnums(const char**);
    void			setEnums(const EnumDef&);
				//!<Enumdef is assumed to remain in memory

    void			fillDefStr(BufferString&) const override;
    bool			isSet() const;
    void			setSet(bool yn=true);
};


/*!
\brief String parameter that is used by an attribute.
*/

mExpClass(AttributeEngine) StringParam : public ValParam
{ mODTextTranslationClass(StringParam);
public:
				StringParam(const char* key);
				StringParam(const char* key,const char* defstr,
					    bool isreq=true);
				~StringParam();

    StringParam*		clone() const override;

    bool			setCompositeValue(const char*) override;
    bool			getCompositeValue(BufferString&) const override;
    BufferString		getDefaultValue() const override
				{ return getDefaultStringValue(); }
};


/*!
\brief Numerical parameter that is used by an attribute.
For example: IntParam, FloatParam and DoubleParam.
*/

template <class T>
mClass(AttributeEngine) NumParam : public ValParam
{ mODTextTranslationClass(NumParam);
public:
				NumParam(const char* key);
				NumParam(const char* key,T defval,
					 bool isreq=true);
				NumParam(const NumParam<T>&);
				~NumParam();

    NumParam<T>*		clone() const override
				{ return new NumParam<T>(*this); }

    void			setLimits(const Interval<T>&);
    void			setLimits(const StepInterval<T>&);
    void			setLimits(T start,T stop,T step=1);
    const StepInterval<T>*	limits() const;
    bool			getCompositeValue(
					BufferString& res) const override;
    bool			setCompositeValue(const char*) override;

    int				getIntValue(int idx=0) const override;
    float			getFValue(int idx=0) const override;
    double			getDValue(int idx=0) const override;
    BufferString		getDefaultValue() const override;
};


template <class T>
NumParam<T>::NumParam( const char* key )
    : ValParam(key,new NumInpSpec<T>())
{}


template <class T>
NumParam<T>::NumParam( const NumParam<T>& np )
    : ValParam(np.key_,np.spec_->clone())
{
    enabled_ = np.enabled_;
    required_ = np.required_;
}


template <class T>
NumParam<T>::NumParam( const char* key, T defval, bool isreq )
    : ValParam( key, new NumInpSpec<T>() )
{
    setValue( defval );
    setDefaultValue( defval );
    required_ = isreq;
}


template <class T>
NumParam<T>::~NumParam()
{}


template <class T>
bool NumParam<T>::getCompositeValue( BufferString& res ) const
{
    res = spec_ && !spec_->isUndef() ? spec_->text() : "1e30";
    return spec_;
}


template <class T>
float NumParam<T>::getFValue( int idx ) const
{
    if ( !spec_ ) return mUdf(float);
    float res = spec_->isUndef() ? mUdf(float) : ValParam::getFValue( idx );
    return res;
}

template <class T>
int NumParam<T>::getIntValue( int idx ) const
{
    if ( !spec_ ) return mUdf(int);
    int res = spec_->isUndef() ? mUdf(int) : ValParam::getIntValue( idx );
    return res;
}


template <class T>
double NumParam<T>::getDValue( int idx ) const
{
    if ( !spec_ ) return mUdf(double);
    double res = spec_->isUndef() ? mUdf(double) : ValParam::getDValue( idx );
    return res;
}

template <class T>
bool NumParam<T>::setCompositeValue( const char* nv )
{
    spec_->setText(nv,0);
    return true;
}


template <class T>
void NumParam<T>::setLimits( const Interval<T>& limit )
{ setLimits( StepInterval<T>(limit.start,limit.stop,(T)1) ); }

template <class T>
void NumParam<T>::setLimits( const StepInterval<T>& limit )
{ reinterpret_cast<NumInpSpec<T>*>(spec_)->setLimits( limit ); }

template <class T>
void NumParam<T>::setLimits( T start, T stop, T step )
{ setLimits( StepInterval<T>( start, stop, step ) ); }

template <class T>
const StepInterval<T>* NumParam<T>::limits() const
{ return reinterpret_cast<NumInpSpec<T>*>(spec_)->limits(); }


template <class T>
BufferString NumParam<T>::getDefaultValue() const
{
    BufferString res =
	toString(reinterpret_cast<NumInpSpec<T>*>(spec_)->defaultValue() );
    return res;
}


typedef NumParam<int>		IntParam;
typedef NumParam<float>		FloatParam;
typedef NumParam<double>	DoubleParam;


/*!
\brief Gate parameter that is used by an attribute.
For example: IntGateParam, FloatGateParam, DoubleGateParam and ZGateParam.
*/

template <class T>
mClass(AttributeEngine) NumGateParam : public ValParam
{ mODTextTranslationClass(NumGateParam);
public:
				NumGateParam(const char* key);
				NumGateParam(const char* key,
					     const Interval<T>& defaultgate,
					     bool isreq=true);
				NumGateParam(const NumGateParam<T>&);
				~NumGateParam();

    NumGateParam<T>*		clone() const override
				{ return new NumGateParam<T>(*this); }

    void			setLimits(const Interval<T>&);
    void			setLimits(T start,T stop);
    bool			getCompositeValue(
					BufferString& res) const override;
    bool			setCompositeValue(const char*) override;

    void			setValue(const Interval<T>&);
    Interval<T>			getValue() const;
    void			setDefaultValue(const Interval<T>&);
    BufferString		getDefaultValue() const override;
    Interval<T>			getDefaultGateValue() const;

    void			toString(BufferString&,
					 const Interval<T>&) const;
};


template <class T>
NumGateParam<T>::NumGateParam( const char* key )
    : ValParam(key,new NumInpIntervalSpec<T>())
{}


template <class T>
NumGateParam<T>::NumGateParam( const NumGateParam<T>& np )
    : ValParam(np.key_,np.spec_->clone())
{
    enabled_ = np.enabled_;
    required_ = np.required_;
}


template <class T>
NumGateParam<T>::NumGateParam( const char* key, const Interval<T>& defaultgate,
			       bool isreq )
    : ValParam(key,new NumInpIntervalSpec<T>())
{
    setValue( defaultgate );
    setDefaultValue( defaultgate );
    setRequired( isreq );
}


template <class T>
NumGateParam<T>::~NumGateParam()
{}


template <class T>
bool NumGateParam<T>::getCompositeValue( BufferString& res ) const
{
    NumInpIntervalSpec<T>* sp = reinterpret_cast<NumInpIntervalSpec<T>*>(spec_);
    if ( !sp ) return false;

    Interval<T> intv( sp->value(0), sp->value(1) );
    toString( res, intv );
    return true;
}


template <class T>
Interval<T> NumGateParam<T>::getValue() const
{
    NumInpIntervalSpec<T>* spec =
				reinterpret_cast<NumInpIntervalSpec<T>*>(spec_);
    if ( !spec ) return Interval<T>(mUdf(T),mUdf(T));
    return Interval<T>( spec->value(0), spec->value(1) );
}


template <class T>
bool NumGateParam<T>::setCompositeValue( const char* gatestr )
{
    if ( !gatestr || !*gatestr )
	return false;

    BufferString rgstr( gatestr );
    rgstr.unEmbed( '[', ']' );
    if ( rgstr.isEmpty() )
	return false;
    char* ptrval2 = rgstr.find( ',' );
    if ( !ptrval2 )
	return false;

    *ptrval2++ = '\0';
    spec_->setText( rgstr.buf(), 0 );
    spec_->setText( ptrval2, 1 );
    return true;
}


template <class T>
void NumGateParam<T>::setLimits( const Interval<T>& limit )
{ reinterpret_cast<NumInpIntervalSpec<T>*>(spec_)->setLimits( limit ); }


template <class T>
void NumGateParam<T>::setLimits( T start, T stop )
{ reinterpret_cast<NumInpIntervalSpec<T>*>(spec_)->setLimits(
						Interval<T>(start,stop) ); }


template <class T>
void NumGateParam<T>::setValue( const Interval<T>& gate )
{ reinterpret_cast<NumInpIntervalSpec<T>*>(spec_)->setValue( gate ); }


template <class T>
void NumGateParam<T>::setDefaultValue( const Interval<T>& defgate )
{ reinterpret_cast<NumInpIntervalSpec<T>*>(spec_)->setDefaultValue( defgate ); }


template <class T>
Interval<T> NumGateParam<T>::getDefaultGateValue() const
{
    NumInpIntervalSpec<T>* spec =
				reinterpret_cast<NumInpIntervalSpec<T>*>(spec_);
    return Interval<T>( spec->defaultValue(0), spec->defaultValue(1) );
}


template <class T>
BufferString NumGateParam<T>::getDefaultValue() const
{
    Interval<T> intv = getDefaultGateValue();
    BufferString res;
    toString( res, intv );
    return res;
}


template <class T>
void NumGateParam<T>::toString(BufferString& res, const Interval<T>& gate) const
{
    res.set( "[" ).add( gate.start ).add( "," ).add( gate.stop ).add( "]" );
}


typedef NumGateParam<int>		IntGateParam;
typedef NumGateParam<float>		FloatGateParam;
typedef NumGateParam<double>		DoubleGateParam;

#define mLargestZGate 10000
typedef NumGateParam<float>		ZGateParam;


/*!
\brief Stored seismic input parameter used by an attribute.
*/

mExpClass(AttributeEngine) SeisStorageRefParam : public StringParam
{ mODTextTranslationClass(SeisStorageRefParam);
public:
				SeisStorageRefParam();
				~SeisStorageRefParam();

    SeisStorageRefParam*	clone() const override;
    bool			isOK() const override;
};

} // namespace Attrib
