#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.12 2005-08-04 10:01:58 cvsnanne Exp $
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

    const char*			getKey() const		{ return key_.buf(); }


    virtual bool		setCompositeValue( const char* ) 
				{ return false; }
    virtual bool		setValues( BufferStringSet& )
				{ return false; }
    virtual bool		getCompositeValue(BufferString&) const	=0;
    
    void			setDefaultValue(BufferString val)
    				{ defaultval_ = val; }
    BufferString		getDefaultValue() { return defaultval_; }
    void			setKey( char* newkey ) { key_ = newkey; }

protected:

    BufferString		key_;
    bool			isgroup_;

    bool			enabled_;
    bool			required_;
    BufferString		defaultval_;

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
    int				getIntValue(int idx=0) const;
    float			getfValue(int idx=0) const;
    bool			getBoolValue(int idx=0) const;
    const char*			getStringValue(int idx=0) const;

    void			setValue(int,int idx=0);
    void			setValue(float,int idx=0);
    void			setValue(bool,int idx=0);
    void			setValue(const char*,int idx=0);

    DataInpSpec*		getSpec() { return spec_; }

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    
protected:
    DataInpSpec*		spec_;

    virtual bool		isEqual(const Param&) const;
};


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
    void 			setValue(const Interval<float>& gate);
};


class BinIDParam : public ValParam
{
public:
    				BinIDParam(const char*);
    BinIDParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

    void                        setDefaultValue(const BinID&);
};


class BoolParam : public ValParam
{
public:
    				BoolParam(const char*);
    BoolParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    void                        setDefaultValue(bool);
};


class EnumParam : public ValParam
{
public:
    				EnumParam(const char*);
    EnumParam*			clone() const;
    void			addEnum(const char*);
    void			addEnums(const char**);
};


class StringParam : public ValParam
{
public:
    				StringParam(const char* key);
    StringParam*		clone() const;

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
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
};


template <class T>
NumParam<T>::NumParam( const NumParam<T>& np )
    : ValParam(np.key_,np.spec_->clone())
{
    enabled_ = np.enabled_;
    required_ = np.required_;
    defaultval_ = np.defaultval_;
}


template <class T>
bool NumParam<T>::getCompositeValue( BufferString& res ) const
{
    if ( !spec_ ) return false;
    res = spec_->isUndef() ? sKey::FloatUdf : spec_->text();
    return true;
}


template <class T>
void NumParam<T>::setLimits( const Interval<T>& limit )
{ reinterpret_cast<NumInpSpec<T>*>(spec_)->setLimits( limit ); }


typedef NumParam<int>		IntParam;
typedef NumParam<float>		FloatParam;
typedef NumParam<double>	DoubleParam;


class SeisStorageRefParam : public StringParam
{
public:
				SeisStorageRefParam(const char* key);
    SeisStorageRefParam*	clone() const;
    bool			isOK() const;
};


}; // namespace Attrib


#endif
