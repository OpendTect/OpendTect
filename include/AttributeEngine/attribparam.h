#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.26 2006-08-15 17:37:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "seistype.h"
#include "datainpspec.h"

class DataInpSpec;
class BinID;
class BufferStringSet;

template <class T> class Interval;

namespace Attrib
{

/*! A parameter that is used by an attribute.

Each attribute has a defenition string that defines how the attribute is
computed. The defenition string has the format:

AttribNameWithoutSpaces param1=value1 param2=value2,value3

The paramater thus has a key (e.g. param1) and one or more associated values.
*/

class Param
{
public:
    				Param(const char* key);
    				Param(const Param&);
    virtual			~Param() {}

    virtual Param*		clone() const		= 0;

    bool			operator==( const Param& p ) const
				{ return _isEqual( p ); }
    bool			operator!=( const Param& p ) const
				{ return !_isEqual( p ); }

    virtual bool		isOK() const		= 0;

    bool			isEnabled() const	  { return enabled_; }
    void			setEnabled(bool yn=true)  { enabled_=yn; }
    bool			isRequired() const	  { return required_; }
    void			setRequired(bool yn=true) { required_=yn; }
    bool			isGroup() const		  { return isgroup_; }

    const char*			getKey() const		  { return key_.buf(); }

    				/*!Set all values from one composite string.*/
    virtual bool		setCompositeValue(const char*) 
				{ return false; }
    				/*!Set all values from multiple strings.*/
    virtual bool		setValues(BufferStringSet&)
				{ return false; }
    virtual bool		getCompositeValue(BufferString&) const	=0;
    				/*!<Put all values into one string. */
    
    virtual BufferString	getDefaultValue() const	  { return ""; } 
    void			setKey(const char* newkey)    { key_ = newkey; }

    virtual void		fillDefStr(BufferString&) const		=0;			

protected:

    BufferString		key_;
    bool			isgroup_;

    bool			enabled_;
    bool			required_;

    bool			_isEqual( const Param& p ) const
				{
				    return p.key_!=key_ ? false : isEqual( p );
				}
    virtual bool		isEqual(const Param&) const	= 0;
};


class ValParam : public Param
{
public:
    				ValParam(const char* key,DataInpSpec*);
    				ValParam(const ValParam&);
    				~ValParam(); 

    virtual ValParam*		clone() const;

    virtual bool		isOK() const;

    int				nrValues() const;
    virtual int			getIntValue(int idx=0) const;
    virtual float		getfValue(int idx=0) const;
    bool			getBoolValue(int idx=0) const;
    const char*			getStringValue(int idx=0) const;

    void			setValue(int,int idx=0);
    void			setValue(float,int idx=0);
    void			setValue(bool,int idx=0);
    void			setValue(const char*,int idx=0);

    virtual int			getDefaultIntValue(int idx=0) const;
    virtual float		getDefaultfValue(int idx=0) const;
    bool			getDefaultBoolValue(int idx=0) const;
    const char*			getDefaultStringValue(int idx=0) const;

    void			setDefaultValue(int,int idx=0);
    void			setDefaultValue(float,int idx=0);
    void			setDefaultValue(bool,int idx=0);
    void			setDefaultValue(const char*,int idx=0);

    DataInpSpec*		getSpec()	{ return spec_; }
    const DataInpSpec*		getSpec() const	{ return spec_; }

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    virtual BufferString	getDefaultValue() const	{ return ""; } 
    virtual void   	        fillDefStr(BufferString&) const;
    
protected:
    DataInpSpec*		spec_;

    virtual bool		isEqual(const Param&) const;
};


/*
#define mLargestZGate 10000
class ZGateParam : public ValParam
{
public:
    				ZGateParam(const char*);
    virtual ZGateParam*		clone() const;
    void			setLimits(const Interval<float>&);

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

    void			setDefaultValue(const Interval<float>&);
    Interval<float>		getDefaultZGateValue() const;
    virtual BufferString	getDefaultValue() const; 
    void 			setValue(const Interval<float>& gate);
    Interval<float>		getValue() const;

    void			toString(BufferString&,
	    				 const Interval<float>&) const;
};
*/

class BinIDParam : public ValParam
{
public:
    				BinIDParam(const char*);
    BinIDParam*			clone() const;
    void			setLimits(const Interval<int>& inlrg,
	    				  const Interval<int>& crlrg);

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

    void                        setDefaultValue(const BinID&);
    BinID			getDefaultBinIDValue() const;
    BufferString		getDefaultValue() const;
    BinID			getValue() const;
    
    void			toString(BufferString&,const BinID&) const;
};


class BoolParam : public ValParam
{
public:
    				BoolParam(const char*);
    BoolParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    BufferString		getDefaultValue() const;

    bool			isSet() const;
    void			setSet(bool yn=true);
};


class EnumParam : public ValParam
{
public:
    				EnumParam(const char*);
    EnumParam*			clone() const;
    BufferString		getDefaultValue() const;

    void			addEnum(const char*);
    void			addEnums(const char**);

    void                	fillDefStr(BufferString&) const;
    bool			isSet() const;
    void			setSet(bool yn=true);
};


class StringParam : public ValParam
{
public:
    				StringParam(const char* key);
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
				NumParam(const NumParam<T>&);

    virtual NumParam<T>*	clone() const
				{ return new NumParam<T>(*this); }

    void			setLimits(const Interval<T>&);
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
{ reinterpret_cast<NumInpSpec<T>*>(spec_)->setLimits( limit ); }


template <class T>
BufferString NumParam<T>::getDefaultValue() const
{
    BufferString bfs = reinterpret_cast<NumInpSpec<T>*>(spec_)->defaultValue();
    return bfs;
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
				
				NumGateParam(const NumGateParam<T>&);

    virtual NumGateParam<T>*	clone() const
				{ return new NumGateParam<T>(*this); }

    void			setLimits(const Interval<T>&);
    virtual bool		getCompositeValue(BufferString& res) const;
    virtual bool                setCompositeValue(const char*);

    void                        setValue(const Interval<T>&);
    Interval<T>			getValue() const;
    void			setDefaultValue(const Interval<T>&);
    virtual BufferString	getDefaultValue() const;
    Interval<T>             	getDefaultGateValue() const;

    void                        toString(BufferString&,
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


class SeisStorageRefParam : public StringParam
{
public:
				SeisStorageRefParam(const char* key);
    SeisStorageRefParam*	clone() const;
    bool			isOK() const;
};


}; // namespace Attrib


#endif
