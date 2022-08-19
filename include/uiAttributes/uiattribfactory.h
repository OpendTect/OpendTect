#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "bufstringset.h"

class uiAttrDescEd;
class uiParent;

/*! \brief Factory for attrib editors.  */


typedef uiAttrDescEd* (*uiAttrDescEdCreateFunc)(uiParent*,bool);

mExpClass(uiAttributes) uiAttributeFactory
{
public:
    virtual		~uiAttributeFactory();

    int			add(const char* displaynm,const char* attrnm,
			    const char* grpnm,uiAttrDescEdCreateFunc,int,int);
    bool		enable(const char* attrnm,bool yn=true);
    bool		remove(const char* attrnm);
    uiAttrDescEd*	create(uiParent*,const char* attrnm, bool,
			       bool dispnm=true) const;

    int			size() const	{ return entries_.size(); }
    const char*		getAttribName( int idx ) const
					{ return entries_[idx]->attrnm_; }
    const char*		getDisplayName( int idx ) const
					{ return entries_[idx]->dispnm_; }
    const char*		getGroupName( int idx ) const
					{ return entries_[idx]->grpnm_; }
    int			domainType( int idx ) const
					{ return entries_[idx]->domtyp_; }
				//!< Is, in fact, uiAttrDescEd::DomainType
				//!< Not used to avoid dependency
    int			dimensionType( int idx ) const
					{ return entries_[idx]->dimtyp_; }
				//!< Is, in fact, uiAttrDescEd::DimensionType
				//!< Not used to avoid dependency

    const char*		dispNameOf(const char*) const;
    const char*		attrNameOf(const char*) const;
    bool		isPresent(const char*,bool dispnm) const;
    bool		isEnabled(const char*,bool dispnm) const;
    bool		hasSteering() const;

protected:

    struct Entry
    {
				Entry(	const char* dn, const char* an,
					const char* gn,
					uiAttrDescEdCreateFunc fn,
					int dt, int dimtyp )
				    : dispnm_(dn)
				    , attrnm_(an)
				    , grpnm_(gn)
				    , domtyp_(dt)
				    , dimtyp_(dimtyp)
				    , crfn_(fn)		{}

	BufferString		dispnm_;
	BufferString		attrnm_;
	BufferString		grpnm_;
	int			domtyp_;
	int			dimtyp_;
	bool			enabled_ = true;
	uiAttrDescEdCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char* nm,bool dispnm) const;

    friend mGlobal(uiAttributes) uiAttributeFactory&	uiAF();
    void			fillStd();
};

mGlobal(uiAttributes) uiAttributeFactory& uiAF();
