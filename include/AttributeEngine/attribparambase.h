#ifndef attribparambase_h
#define attribparambase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          16-01-2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
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

    FixedString			getKey() const		  { return key_.buf(); }

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

}; // namespace Attrib


#endif

