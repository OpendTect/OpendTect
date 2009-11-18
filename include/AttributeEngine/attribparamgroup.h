#ifndef attribparamgroup_h
#define attribparamgroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: attribparamgroup.h,v 1.9 2009-11-18 05:33:45 cvsnanne Exp $
________________________________________________________________________
*/

/*!\brief Attrib::Param that contains many (a set) Attrib::Param of one kind.

 It's made for parameters where you don't know how many are required. 
 An example can be positions, where the number of positions are unknown at 
 the beginning of parsing.

The Attrib::ParamGroup have a prefix and constructs parameter names on the form
prefix + figure:

pos0, pos1, ..., posN.

The Attrib::ParamGroup is constructed by:
Attrib::ParamGroup( int startsz, const char* prefix, const PT& templ ),

where startsz is the number of instances at the beginning, templ is a
a template for the parameters that should be constructed.

-*/

#include "attribparam.h"
#include "datainpspec.h"
#include "bufstringset.h"
#include <stdio.h>

namespace Attrib
{

template <class PT>
class ParamGroup : public Param
{
public:

    			ParamGroup( int startsz, const char* prefix,
				    const PT& templ);
			ParamGroup(const ParamGroup&);
			~ ParamGroup();

    ParamGroup<PT>*	clone() const;
    bool		isOK() const;
    const char*		errMsg() const;
    bool                setValues(BufferStringSet&);
    bool                getCompositeValue(BufferString&) const;
    void                fillDefStr(BufferString&) const;

    Param&		operator[]( int idx )		{ return *params[idx]; }
    const Param&	operator[]( int idx ) const	{ return *params[idx]; }

    void		setSize( int );
    int			size() const { return sz; }
    const char*		getPrefix() const { return prefix; }

protected:
    int				getSize() const;
    bool			isEqual(const Param&) const;

    int				sz;

    const char*			prefix;
    PT				templ;
    ObjectSet<PT>		params;
    ObjectSet<char>		keys;

    BufferString		errmsg;

};


#define mDescGetParamGroup(PT,ret,desc,key) \
ParamGroup<PT>* ret; \
{ \
    Param* param = desc.getParam( key ); \
    mDynamicCastGet(ParamGroup<PT>*,tmp,param) \
    ret = tmp; \
}

#define mDescGetConstParamGroup(PT,ret,desc,key) \
const ParamGroup<PT>* ret; \
{\
    const Param* param = desc.getParam( key ); \
    mDynamicCastGet(const ParamGroup<PT>*,tmp,param)\
    ret = tmp; \
}


/*
template <class PT>
inline const ParamGroup<PT>* getParamGroup( const Desc& desc, const char* key )
{
    return getParamGroup( const_cast<Desc&>( desc ), key );
}
*/

template <class PT> inline
bool ParamGroup<PT>::isEqual(const Param& b) const
{
    mDynamicCastGet(ParamGroup<PT>*,pgr,&const_cast<Param&>(b));
    if ( pgr->size() != sz ) return false;
    if ( strcmp( pgr->getPrefix(), prefix )) return false;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( params[idx]->getSpec()->nElems()
		!= pgr->params[idx]->getSpec()->nElems() )
	    return false;

	for ( int idy=0; 
		idy<params[idx]->getSpec()->nElems(); idy++ )
	{
	    BufferString txt( params[idx]->getSpec()->text(idx) );
	    if ( txt != pgr->params[idx]->getSpec()->text(idx) )
		return false;
	}
    }
    return true;
}

 
template <class PT> inline
bool ParamGroup<PT>::isOK() const
{
    if ( !enabled_ ) return true;
    if ( !sz ) return false;
    
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !params[idx]->isOK() )
	{
	    BufferString& err = const_cast<ParamGroup*>(this)-> errmsg;
	    err = "cannot parse parameter "; err += idx; 
	    err += " of the group "; err += prefix;
	    return false;
	}
    }

    return true;
}


template <class PT> inline
bool ParamGroup<PT>::setValues( BufferStringSet& vals )
{
    setSize( vals.size() );
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !params[idx]->setCompositeValue(vals.get(idx) ) )
	    return false;
    }

    return isOK();
}


template <class PT> inline
ParamGroup<PT>* ParamGroup<PT>::clone() const
{
    return new ParamGroup<PT>(*this);
}


template <class PT> inline
const char* ParamGroup<PT>::errMsg() const
{ return errmsg; }


template <class PT> inline
ParamGroup<PT>::ParamGroup( int startsz, const char* prefix_, const PT& templ_)
    : Param ( prefix_ )
    , templ( templ_ )
    , prefix( prefix_ )
{
    isgroup_ = true;
    setSize( startsz );
}


template <class PT> inline
ParamGroup<PT>::ParamGroup( const ParamGroup<PT>& a )
    : Param ( a )
    , prefix( a.prefix )
    , templ( a.templ )
    , sz( a.sz )
{
    for ( int idx=0; idx<a.params.size(); idx++ )
    {
	PT* np = new PT( (PT&)a[idx] );
        char* newkey = new char[strlen(prefix) + 10];
        sprintf( newkey, "%s%d", prefix, idx );
        np->setKey( newkey );

        keys += newkey;
	params += np;
    }
}

template <class PT> inline
ParamGroup<PT>::~ParamGroup()
{
    deepErase( params );
    for ( int idx=0; idx<keys.size(); idx++ )
	delete [] keys[idx];
}


template <class PT> inline
int ParamGroup<PT>::getSize() const
{
    return sz;
}


template <class PT> inline
void ParamGroup<PT>::setSize( int nsz )
{
    while ( nsz > params.size() )
    {
	PT* newpara = new PT(templ);

	char* newkey = new char[strlen(prefix) + 10];
	sprintf( newkey, "%s%d", prefix, params.size() );

	newpara->setKey( newkey );

	keys += newkey;
	params += newpara;
    }

    sz = nsz;
}

template <class PT> inline
bool ParamGroup<PT>::getCompositeValue( BufferString& res ) const
{
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString tmpres;
	if ( !params[idx]->getCompositeValue(tmpres) )
	    return false;

	res += tmpres; res +=" ";
    }
    return true;
}


template <class PT> inline
void ParamGroup<PT>::fillDefStr( BufferString& res ) const
{
    for ( int idx=0; idx<sz; idx++ )
    {
	res += params[idx]->getKey();
	res += "=";
	BufferString val;
	if ( !params[idx]->isRequired() || !params[idx]->getCompositeValue(val))
	    val = params[idx]->getDefaultValue();
	res += val;
	res += " ";
    }
}


#define mAttribSpecSet(sizegiver,sizetaker)                             \
    public:                                                             \
	bool            update()                                        \
			{                                               \
			    if ( sizegiver == sizetaker.size() )        \
				return false;                           \
									\
			    sizetaker.setSize( sizegiver );             \
									\
			    return true;                                \
			}


}//namespace

#endif
