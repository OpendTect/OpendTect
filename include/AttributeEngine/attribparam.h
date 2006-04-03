#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.21 2006-04-03 13:24:09 cvshelene Exp $
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

    const char*			getKey() const		  { return key_.buf(); }


    virtual bool		setCompositeValue( const char* ) 
				{ return false; }
    virtual bool		setValues( BufferStringSet& )
				{ return false; }
    virtual bool		getCompositeValue(BufferString&) const	=0;
    
    virtual BufferString	getDefaultValue() const	  { return ""; } 
    void			setKey( char* newkey )    { key_ = newkey; }

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

    DataInpSpec*		getSpec() { return spec_; }

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    virtual BufferString	getDefaultValue() const	{ return ""; } 
    virtual void   	        fillDefStr(BufferString&) const;
    
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
    Interval<float>		getDefaultZGateValue() const;
    virtual BufferString	getDefaultValue() const; 
    void 			setValue(const Interval<float>& gate);
    Interval<float>		getValue() const;

    void			toString(BufferString&,
	    				 const Interval<float>&) const;
};


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
