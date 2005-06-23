#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.7 2005-06-23 09:13:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "seistype.h"

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

    bool			isEnabled() const	  { return enabled; }
    void			setEnabled(bool yn=true)  { enabled=yn; }
    bool			isRequired() const	  { return required; }
    void			setRequired(bool yn=true) { required=yn; }
    bool			isGroup() const		  { return isgroup_; }

    const char*			getKey() const		{ return key.buf(); }


    virtual bool		setCompositeValue( const char* ) 
				{ return false; }
    virtual bool		setValues( BufferStringSet& )
				{ return false; }
    virtual bool		getCompositeValue(BufferString&) const	=0;
    
    void			setDefaultValue(BufferString val)
    				{ defaultval_ = val; }
    BufferString		getDefaultValue() { return defaultval_; }
    void			setKey( char* newkey) { key = newkey; }

protected:

    BufferString		key;
    bool			isgroup_;

    bool			enabled;
    bool			required;
    BufferString		defaultval_;

    bool			_isEqual( const Param& p ) const
				{
				    return p.key != key ? false : isEqual( p );
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

    DataInpSpec*		getSpec() { return spec; }

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    
protected:
    DataInpSpec*		spec;

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
};


class BinIDParam : public ValParam
{
public:
    				BinIDParam(const char*);
    BinIDParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

    void                        setDefaultValue( const BinID& );
};


class BoolParam : public ValParam
{
public:
    				BoolParam(const char*);
    BoolParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
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


class SeisStorageRefParam : public StringParam
{
public:
				SeisStorageRefParam(const char* key);
    SeisStorageRefParam*	clone() const;
    bool			isOK() const;
};


}; // namespace Attrib


#endif
