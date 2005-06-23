#ifndef attribparamgroup_h
#define attribparamgroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: attribparamgroup.h,v 1.1 2005-06-23 09:13:36 cvshelene Exp $
________________________________________________________________________
*/

/*!\brief AttribParam that contains many (a set) AttribParam of one kind.

 It's made for parameters where you don't know how many are required. 
 An example can be positions, where the number of positions are unknown at 
 the beginning of parsing.

The AttribParamGroup have a prefix and constructs parameter names on the form
prefix + figure:

pos0, pos1, ..., posN.

The AttribParamGroup is constructed by:
 AttribParamGroup( int startsz, const char* prefix, const PT& templ ),

where startsz is the number of instances at the beginning, templ is a
a template for the parameters that should be constructed.

-*/

#include "attribparam.h"
#include "datainpspec.h"
#include "bufstringset.h"
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
	    if ( strcmp( params[idx]->getSpec()->text(idx),
		    	pgr->params[idx]->getSpec()->text(idx)) )
		return false;
	}
    }
    return true;
}

 
template <class PT> inline
bool ParamGroup<PT>::isOK() const
{
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
	if ( !params[idx]->getSpec()->setText(vals.get(idx),0) )
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
    : Param ( a.prefix )
    , prefix( a.prefix )
    , templ( a.templ )
    , sz( a.sz )
{
    isgroup_ = true;
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
