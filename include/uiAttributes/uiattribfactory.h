#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "bufstring.h"
#include "uistring.h"

class uiAttrDescEd;
class uiParent;

/*! \brief Factory for attrib editors.  */


typedef uiAttrDescEd* (*uiAttrDescEdCreateFunc)(uiParent*,bool);

mExpClass(uiAttributes) uiAttributeFactory
{
public:

    uiAttrDescEd*	create(uiParent*,const char* nm,bool) const;
    uiAttrDescEd*	create(uiParent*,const uiString& nm,bool) const;

    int			size() const	{ return entries_.size(); }
    const char*		getAttribName(int) const;
    const uiString&	getDisplayName(int) const;
    const uiString&	getGroupName(int) const;
    int			domainType(int) const;
				//!< Is, in fact, uiAttrDescEd::DomainType
				//!< Not used to avoid dependency
    int			dimensionType(int) const;
				//!< Is, in fact, uiAttrDescEd::DimensionType
				//!< Not used to avoid dependency
    bool		isSyntheticSupported(int) const;
    bool		isGroupDef(int) const;

    const uiString&	dispNameOf(const char*) const;
    const char*		attrNameOf(const uiString&) const;

    int			indexOf(const char* nm) const;
    int			indexOf(const uiString& nm) const;
    inline bool		isPresent( const char* nm ) const
			{ return indexOf(nm) >= 0; }
    inline bool		isPresent( const uiString& nm ) const
			{ return indexOf(nm) >= 0; }

protected:

    struct Entry
    {
			Entry(	const uiString& dn, const char* an,
				const uiString& gn,
				uiAttrDescEdCreateFunc fn,
				int dt, int dimtyp, bool supsynth, bool grpdef )
			    : dispnm_(dn)
			    , attrnm_(an)
			    , grpnm_(gn)
			    , domtyp_(dt)
			    , dimtyp_(dimtyp)
			    , supportsynthetic_(supsynth)
			    , isgroupdef_(grpdef)
			    , crfn_(fn)		{}

	BufferString	attrnm_;
	uiString	dispnm_;
	uiString	grpnm_;
	int		domtyp_;
	int		dimtyp_;
	bool		supportsynthetic_;
	bool		isgroupdef_;
	uiAttrDescEdCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*) const;
    Entry*		getEntry(const uiString&) const;

    friend mGlobal(uiAttributes) uiAttributeFactory&	uiAF();
    void		fillStd();

public:

    virtual		~uiAttributeFactory();

    int			add(const uiString& displaynm,const char* attrnm,
			    const uiString& grpnm,uiAttrDescEdCreateFunc,
			    int,int,bool synth,bool isgrpdef);

    inline bool		haveSteering() const
			{ return isPresent( "Curvature" ); }

};

mGlobal(uiAttributes) uiAttributeFactory& uiAF();
