#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          16-01-2008
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "idxpair.h"
#include "bufstring.h"

class DataInpSpec;
class BufferStringSet;

namespace Attrib
{

/*!
\brief A parameter that is used by an attribute.

  Each attribute has a definition string that defines how the attribute is
  computed. The definition string has the format:

  AttribNameWithoutSpaces param1=value1 param2=value2,value3

  The parameter thus has a key (e.g. param1) and one or more associated values.
*/

mExpClass(AttributeEngine) Param
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

    const char*			getKey() const		  { return key_.str(); }

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


/*!
\brief Attribute Value Parameter
*/

mExpClass(AttributeEngine) ValParam : public Param
{
public:
				ValParam(const char* key,DataInpSpec*);
				ValParam(const ValParam&);
				~ValParam();

    virtual ValParam*		clone() const;

    virtual bool		isOK() const;

    int				nrValues() const;
    virtual int			getIntValue(int idx=0) const;
    virtual float		getFValue(int idx=0) const;
    virtual double		getDValue(int idx=0) const;
    bool			getBoolValue(int idx=0) const;
    const char*			getStringValue(int idx=0) const;
    template<class IdxPairT>
    inline IdxPairT		getIdxPairValue() const
				{ return IdxPairT( getIntValue(0),
						   getIntValue(1) ); }

    void			setValue(int,int idx=0);
    void			setValue(float,int idx=0);
    void			setValue(bool,int idx=0);
    void			setValue(const char*,int idx=0);
    void			setValue(double,int idx=0);
    inline void			setValue( const IdxPair& ip )
				{ setValue(ip.first(),0);
				  setValue(ip.second(),1); }

    virtual int			getDefaultIntValue(int idx=0) const;
    virtual float		getDefaultFValue(int idx=0) const;
    bool			getDefaultBoolValue(int idx=0) const;
    const char*			getDefaultStringValue(int idx=0) const;
    virtual double		getDefaultDValue(int idx=0) const;
    template<class IdxPairT>
    inline IdxPairT		getDefaultIdxPairValue() const
				{ return IdxPairT( getDefaultIntValue(0),
						   getDefaultIntValue(1) ); }

    void			setDefaultValue(int,int idx=0);
    void			setDefaultValue(float,int idx=0);
    void			setDefaultValue(bool,int idx=0);
    void			setDefaultValue(const char*,int idx=0);
    void			setDefaultValue(double,int idx=0);
    inline void			setDefaultValue( const IdxPair& ip )
				{ setDefaultValue(ip.first(),0);
				  setDefaultValue(ip.second(),1); }

    DataInpSpec*		getSpec()	{ return spec_; }
    const DataInpSpec*		getSpec() const	{ return spec_; }

    virtual bool		setCompositeValue(const char*);
    virtual bool		getCompositeValue(BufferString&) const;
    virtual BufferString	getDefaultValue() const	{ return ""; }
    virtual void	        fillDefStr(BufferString&) const;

protected:

    DataInpSpec*		spec_;

    virtual bool		isEqual(const Param&) const;

public:

    mDeprecated float		getfValue( int idx=0 ) const
				{ return getFValue( idx ); }
    mDeprecated double		getdValue( int idx=0 ) const
				{ return getDValue( idx ); }
    mDeprecated float		getDefaultfValue( int idx=0 ) const
				{ return getDefaultFValue( idx ); }
    mDeprecated double		getDefaultdValue( int idx=0 ) const
				{ return getDefaultDValue( idx ); }

};

} // namespace Attrib
