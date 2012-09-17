#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.35 2011/02/03 21:31:33 cvskris Exp $
________________________________________________________________________

-*/

#include "attribparambase.h"
#include "datainpspec.h"

class BinID;

template <class T> class Interval;

namespace Attrib
{

/*! A parameter that is used by an attribute.

Each attribute has a defenition string that defines how the attribute is
computed. The defenition string has the format:

AttribNameWithoutSpaces param1=value1 param2=value2,value3

The paramater thus has a key (e.g. param1) and one or more associated values.

Subclasses are used to provide accurate parameter definition for parameters
of each and every type
*/

mClass BinIDParam : public ValParam
{
public:
    				BinIDParam(const char*);
    				BinIDParam(const char*,const BinID&,
					   bool isreq=true);
    BinIDParam*			clone() const;
    void			setLimits(const Interval<int>& inlrg,
	    				  const Interval<int>& crlrg);
    void			setLimits(int mininl,int maxinl,
	    				  int mincrl,int maxcrl);

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

    void                        setDefaultValue(const BinID&);
    BinID			getDefaultBinIDValue() const;
    BufferString		getDefaultValue() const;
    BinID			getValue() const;
    
    void			toString(BufferString&,const BinID&) const;
};


mClass BoolParam : public ValParam
{
public:
    				BoolParam(const char*);
    				BoolParam(const char*,bool, bool isreq=true);
    BoolParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    BufferString		getDefaultValue() const;

    bool			isSet() const;
    void			setSet(bool yn=true);
};


mClass EnumParam : public ValParam
{
public:
    				EnumParam(const char*);
    				EnumParam(const char*,int defval,
					  bool isreq=true);
    EnumParam*			clone() const;
    BufferString		getDefaultValue() const;

    void			addEnum(const char*);
    void			addEnums(const char**);

    void                	fillDefStr(BufferString&) const;
    bool			isSet() const;
    void			setSet(bool yn=true);
};


mClass StringParam : public ValParam
{
public:
    				StringParam(const char* key);
    				StringParam(const char* key,const char* defstr,
					    bool isreq=true);
    StringParam*		clone() const;

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    virtual BufferString	getDefaultValue() const
				{ return getDefaultStringValue(); }
};


template <class T>
class NumParam : public ValParam
{
public:
    				NumParam(const char* key)
				    : ValParam(key,new NumInpSpec<T>()) {}
    				NumParam(const char* key,T defval,
					 bool isreq=true);
				NumParam(const NumParam<T>&);

    virtual NumParam<T>*	clone() const
				{ return new NumParam<T>(*this); }

    void			setLimits(const Interval<T>&);
    void			setLimits(const StepInterval<T>&);
    void			setLimits(T start,T stop,T step=1);
    const StepInterval<T>*	limits() const;
    virtual bool		getCompositeValue(BufferString& res) const;
    virtual bool                setCompositeValue(const char*);

    virtual int			getIntValue(int idx=0) const;
    virtual float		getfValue(int idx=0) const;
    virtual BufferString	getDefaultValue() const;
};


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
bool NumParam<T>::getCompositeValue( BufferString& res ) const
{
    res = spec_ && !spec_->isUndef() ? spec_->text() : "1e30";
    return spec_;
}


template <class T>
float NumParam<T>::getfValue( int idx ) const
{
    if ( !spec_ ) return mUdf(float);
    float res = spec_->isUndef() ? mUdf(float) : ValParam::getfValue( idx );
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


template <class T>
class NumGateParam : public ValParam
{
public:
    				NumGateParam(const char* key)
				    : ValParam(key,new NumInpIntervalSpec<T>())
				    {}
				
    				NumGateParam(const char* key,
					     const Interval<T>& defaultgate,
					     bool isreq=true);
				    
				NumGateParam(const NumGateParam<T>&);

    virtual NumGateParam<T>*	clone() const
				{ return new NumGateParam<T>(*this); }

    void			setLimits(const Interval<T>&);
    void			setLimits(T start,T stop);
    virtual bool		getCompositeValue(BufferString& res) const;
    virtual bool		setCompositeValue(const char*);

    void			setValue(const Interval<T>&);
    Interval<T>			getValue() const;
    void			setDefaultValue(const Interval<T>&);
    virtual BufferString	getDefaultValue() const;
    Interval<T>			getDefaultGateValue() const;

    void			toString(BufferString&,
					 const Interval<T>&) const;
};


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
bool NumGateParam<T>::setCompositeValue( const char* gatestrvar )
{
    bool res = false;
    const int gatestrsz = strlen( gatestrvar );
    if ( gatestrvar[0] == '[' &&  gatestrvar[gatestrsz-1] == ']' )
    {
	int idx = 0;
	for ( ; idx < gatestrsz; idx ++)
	    if ( gatestrvar[idx] == ',' )
		break;

	if ( idx<gatestrsz )
	{
	    ArrPtrMan<char> valuestr = new char[gatestrsz];
	    strncpy( valuestr, &gatestrvar[1], idx - 1 );
	    valuestr[idx-1] = 0;
	    spec_->setText( valuestr, 0 );

	    strncpy( valuestr, &gatestrvar[idx+1], gatestrsz - idx - 2 );
	    valuestr[gatestrsz - idx - 2] = 0;
	    spec_->setText( valuestr, 1 );

	    res = true;
	}
    }

    return res;
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
    res = "[";
    res += gate.start;
    res += ",";
    res += gate.stop;
    res += "]";
}


typedef NumGateParam<int>		IntGateParam;
typedef NumGateParam<float>		FloatGateParam;
typedef NumGateParam<double>		DoubleGateParam;

#define mLargestZGate 10000
typedef NumGateParam<float>		ZGateParam;


mClass SeisStorageRefParam : public StringParam
{
public:
				SeisStorageRefParam(const char* key);
    SeisStorageRefParam*	clone() const;
    bool			isOK() const;
};


}; // namespace Attrib


#endif
