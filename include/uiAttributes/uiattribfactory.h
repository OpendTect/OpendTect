#ifndef uiattrfact_h
#define uiattrfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiattribfactory.h,v 1.8 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

class uiAttrDescEd;
class uiParent;

/*! \brief Factory for attrib editors.  */


typedef uiAttrDescEd* (*uiAttrDescEdCreateFunc)(uiParent*,bool);

mClass uiAttributeFactory
{
public:
    virtual		~uiAttributeFactory();

    int			add(const char* displaynm,const char* attrnm,
	    		    const char* grpnm,uiAttrDescEdCreateFunc,int);
    uiAttrDescEd*	create(uiParent*,const char* nm, bool,
	    		       bool dispnm=true) const;

    int			size() const	{ return entries_.size(); }
    const char*		getDisplayName( int idx ) const
					{ return entries_[idx]->dispnm_; }
    const char*		getGroupName( int idx ) const
					{ return entries_[idx]->grpnm_; }
    int			domainType( int idx ) const
					{ return entries_[idx]->domtyp_; }
    				//!< Is, in fact, uiAttrDescEd::DomainType
    				//!< Not used to avoid dependency

    const char*		dispNameOf(const char*) const;
    const char*		attrNameOf(const char*) const;

protected:

    struct Entry
    {
				Entry(	const char* dn, const char* an,
					const char* gn,
					uiAttrDescEdCreateFunc fn,
				    	int dt )
				    : dispnm_(dn)
				    , attrnm_(an)
				    , grpnm_(gn)
				    , domtyp_(dt)
				    , crfn_(fn)		{}

	BufferString		dispnm_;
	BufferString		attrnm_;
	BufferString		grpnm_;
	int			domtyp_;
	uiAttrDescEdCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*,bool) const;

    friend mGlobal uiAttributeFactory&	uiAF();
    void			fillStd();
};

mGlobal uiAttributeFactory& uiAF();


#endif
