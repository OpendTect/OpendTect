#ifndef attribparam_h
#define attribparam_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribparam.h,v 1.1 2005-02-01 14:05:34 kristofer Exp $
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
    			Param( const char* key_, DataInpSpec* spec_);
    			Param( const Param& );
    virtual		~Param() {}
    virtual Param*	clone() const;

    bool		isOK() const;

    bool		isEnabled() const;
    void		setEnabled(bool yn=true);

    bool		isRequired() const;
    void		setRequired(bool yn=true);

    const char*		getKey() const;
    const DataInpSpec*	getSpec() const;

    virtual bool	setValue( const char* );
    const char*		getValue() const;

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
    bool		setValue( const char* );
    void		setLimits( const Interval<float>& );

protected:
    virtual bool	getValueString( BufferString& ) const;
};


class BinIDParam : public Param
{
public:
    			BinIDParam( const char* );
    BinIDParam*		clone() const;
    bool		setValue( const char* );

protected:
    virtual bool	getValueString( BufferString& ) const;
};


class EnumParam : public Param
{
public:
    			EnumParam( const char* );
    EnumParam*		clone() const;
    void		addEnum( const char* );
};

/*
typedef void(*ParserStatusUpdater)(Parser&);

class Parser
{ mRefCountImpl(Parser);
public:
    			Parser( const char* name, ParserStatusUpdater=0 );
    Parser*		clone() const;
    void		addParam( Param* );
    const char*		name() const { return basename; }

    bool		init();

    void		getParamNames( BufferStringSet& ) const;

    void		setParamEnabled( const char* key, bool yn=true );
    bool		isParamEnabled( const char* key ) const;
    void		setParamRequired( const char* key, bool yn=true );
    bool		isParamRequired( const char* key ) const;

    const Param*	getParam(const char* key) const;
    bool		setValue(const char* key, const char* value );
    bool		parseDefString( const char* );
    bool		getDefString( BufferString& );

    bool		isOK();

    int			nrInputs() const { return inputs.size(); }
    Seis::DataType	inputType(int idx) const { return inputs[idx]; }
    int			nrOutputs() const { return outputs.size(); }
    Seis::DataType	outputType(int idx) const { return outputs[idx]; }

    static BufferString	getAttribName( const char* defstr );
    static bool		getParamString( const char* defstr, const char* key,
	    				BufferString& );

protected:
    TypeSet<Seis::DataType>	inputs;
    TypeSet<Seis::DataType>	outputs;

    ObjectSet<Param>	params;
    BufferString	basename;
    ParserStatusUpdater	parserupdater;
};

*/

}; //namespace



#endif
