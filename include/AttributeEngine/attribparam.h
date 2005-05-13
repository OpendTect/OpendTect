#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.4 2005-05-13 12:54:06 cvsnanne Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "seistype.h"

class DataInpSpec;
class BufferStringSet;

template <class T> class Interval;

namespace Attrib
{

class Param
{
public:
    				Param(const char* key,DataInpSpec* spec);
    				Param(const Param&);
    virtual			~Param() {}

    virtual Param*		clone() const;

    bool			operator==(const Param&) const;
    bool			operator!=(const Param&) const;

    virtual bool		isOK() const;

    bool			isEnabled() const	  { return enabled; }
    void			setEnabled(bool yn=true)  { enabled=yn; }
    bool			isRequired() const	  { return required; }
    void			setRequired(bool yn=true) { required=yn; }

    const char*			getKey() const;
    int				nrValues() const;
    int				getIntValue(int idx=0) const;
    float			getfValue(int idx=0) const;
    bool			getBoolValue(int idx=0) const;
    const char*			getStringValue(int idx=0) const;

    void			setValue(int,int idx=0);
    void			setValue(float,int idx=0);
    void			setValue(bool,int idx=0);
    void			setValue(const char*,int idx=0);

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;

protected:
    BufferString		key;
    DataInpSpec*		spec;

    bool			enabled;
    bool			required;
};


class ZGateParam : public Param
{
public:
    				ZGateParam(const char*);
    virtual ZGateParam*		clone() const;
    void			setLimits(const Interval<float>&);

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
};


class BinIDParam : public Param
{
public:
    				BinIDParam(const char*);
    BinIDParam*			clone() const;

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
};


class EnumParam : public Param
{
public:
    				EnumParam(const char*);
    EnumParam*			clone() const;
    void			addEnum(const char*);
};


class StringParam : public Param
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
