#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
________________________________________________________________________
*/

#include "attribparam.h"
#include "datainpspec.h"
#include "bufstringset.h"

namespace Attrib
{

/*!
\brief Attrib::Param that contains many (a set) Attrib::Param of one kind.

  It's made for parameters where you don't know how many are required.
  An example can be positions, where the number of positions are unknown at
  the beginning of parsing.

  The Attrib::ParamGroup have a prefix and constructs parameter names on the
  form  prefix + figure:

  pos0, pos1, ..., posN.

  The Attrib::ParamGroup is constructed by:
  Attrib::ParamGroup( int startsz, const char* prefix, const PT& templ ),

  where startsz is the number of instances at the beginning, templ is a
  a template for the parameters that should be constructed.
-*/

template <class PT>
mClass(AttributeEngine) ParamGroup : public Param
{
public:

			ParamGroup( int startsz, const char* prefix,
				    const PT& templ);
			ParamGroup(const ParamGroup&);
			~ParamGroup();

    ParamGroup<PT>*	clone() const override;
    bool		isOK() const override;
    const char*		errMsg() const;
    bool		setValues(BufferStringSet&) override;
    bool		getCompositeValue(BufferString&) const override;
    void		fillDefStr(BufferString&) const override;

    Param&		operator[]( int idx )		{ return *params_[idx];}
    const Param&	operator[]( int idx ) const	{ return *params_[idx];}

    void		setSize(int);
    int			size() const			{ return sz_; }
    int			isEmpty() const			{ return sz_==0; }
    const char*		getPrefix() const		{ return prefix_.buf();}

protected:
    int				getSize() const;
    bool			isEqual(const Param&) const override;

    int				sz_;

    const BufferString		prefix_;
    PT				templ_;
    ObjectSet<PT>		params_;

    BufferString		errmsg_;

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
    if ( !pgr || pgr->size() != sz_ )
	return false;
    if ( prefix_ != pgr->getPrefix() )
	return false;

    for ( int idx=0; idx<sz_; idx++ )
    {
	if ( params_[idx]->getSpec()->nElems()
		!= pgr->params_[idx]->getSpec()->nElems() )
	    return false;

	for ( int idy=0;
		idy<params_[idx]->getSpec()->nElems(); idy++ )
	{
	    const BufferString txt( params_[idx]->getSpec()->text(idx) );
	    if ( txt != pgr->params_[idx]->getSpec()->text(idx) )
		return false;
	}
    }
    return true;
}


template <class PT> inline
bool ParamGroup<PT>::isOK() const
{
    if ( !enabled_ ) return true;
    if ( !sz_ ) return false;

    for ( int idx=0; idx<sz_; idx++ )
    {
	if ( !params_[idx]->isOK() )
	{
	    BufferString& err = const_cast<ParamGroup*>(this)-> errmsg_;
	    err = "cannot parse parameter "; err += idx;
	    err += " of the group "; err += prefix_;
	    return false;
	}
    }

    return true;
}


template <class PT> inline
bool ParamGroup<PT>::setValues( BufferStringSet& vals )
{
    setSize( vals.size() );
    for ( int idx=0; idx<sz_; idx++ )
    {
	if ( !params_[idx]->setCompositeValue(vals.get(idx) ) )
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
{ return errmsg_.str(); }


template <class PT> inline
ParamGroup<PT>::ParamGroup( int startsz, const char* prefix, const PT& templ )
    : Param( prefix )
    , templ_( templ )
    , prefix_( prefix )
{
    isgroup_ = true;
    setSize( startsz );
}


template <class PT> inline
ParamGroup<PT>::ParamGroup( const ParamGroup<PT>& a )
    : Param( a )
    , prefix_( a.prefix_ )
    , templ_( a.templ_ )
    , sz_( a.sz_ )
{
    for ( int idx=0; idx<a.params_.size(); idx++ )
    {
	PT* np = new PT( (PT&)a[idx] );
        np->setKey( BufferString(prefix_,idx) );
	params_ += np;
    }
}

template <class PT> inline
ParamGroup<PT>::~ParamGroup()
{
    deepErase( params_ );
}


template <class PT> inline
int ParamGroup<PT>::getSize() const
{
    return sz_;
}


template <class PT> inline
void ParamGroup<PT>::setSize( int nsz )
{
    while ( nsz > params_.size() )
    {
	PT* newpara = new PT(templ_);
	newpara->setKey( BufferString(prefix_,params_.size()) );
	params_ += newpara;
    }

    sz_ = nsz;
}

template <class PT> inline
bool ParamGroup<PT>::getCompositeValue( BufferString& res ) const
{
    for ( int idx=0; idx<sz_; idx++ )
    {
	BufferString parstr;
	if ( !params_[idx]->getCompositeValue(parstr) )
	    return false;
	res.add( parstr ).add( " " );
    }
    return true;
}


template <class PT> inline
void ParamGroup<PT>::fillDefStr( BufferString& res ) const
{
    for ( int idx=0; idx<sz_; idx++ )
    {
	res.add( params_[idx]->getKey() ).add( "=" );
	BufferString val;
	if ( !params_[idx]->isRequired()
	  || !params_[idx]->getCompositeValue(val))
	    val = params_[idx]->getDefaultValue();
	res.add( val ).add( " " );
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


} // namespace Attrib
