#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.2 2005-02-03 15:35:02 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "seistype.h"
#include "sets.h"

class DataInpSpec;
class BufferStringSet;

template <class T> class Interval;

namespace Attrib
{

class Parser;

class Param
{
public:
    			Param(  const char* key_, DataInpSpec* spec_ );
    			Param( const Param& );
    virtual		~Param() {}

    virtual Param*	clone() const;

    bool		operator==(const Param&) const;
    bool		operator!=(const Param&) const;

    virtual bool	isOK() const;

    bool		isEnabled() const;
    void		setEnabled(bool yn=true);
    bool		isRequired() const;
    void		setRequired(bool yn=true);

    const char*		getKey() const;
    int			nrValues() const;
    int			getIntValue( int idx ) const;
    double		getValue( int idx ) const;
    float		getfValue( int idx ) const;
    bool		getBoolValue( int idx ) const;
    const char*		getStringValue( int idx ) const;


    virtual bool	setCompositeValue( const char* );
    virtual bool	getCompositeValue(BufferString&) const;

protected:
    BufferString	key;
    DataInpSpec*	spec;

    bool		enabled;
    bool		required;
};


class ZGateParam : public Param
{
public:
    			ZGateParam( const char* );
    virtual ZGateParam*	clone() const;
    bool		setCompositeValue( const char* );
    void		setLimits( const Interval<float>& );

protected:
    virtual bool	getCompositeValue(BufferString&) const;
};


class BinIDParam : public Param
{
public:
    			BinIDParam( const char* );
    BinIDParam*		clone() const;
    bool		setCompositeValue( const char* );

protected:
    virtual bool	getCompositeValue(BufferString&) const;
};


class EnumParam : public Param
{
public:
    			EnumParam( const char* );
    EnumParam*		clone() const;
    void		addEnum( const char* );
};


class StringParam : public Param
{
public:
    				StringParam( const char* key );
    StringParam*		clone() const;
    virtual bool		setCompositeValue( const char* );
    virtual bool		getCompositeValue(BufferString&) const;
};


class SeisStorageRefParam : public StringParam
{
public:
				SeisStorageRefParam( const char* key );
    SeisStorageRefParam*	clone() const;
    bool			isOK() const;
};



}; //namespace



#endif
