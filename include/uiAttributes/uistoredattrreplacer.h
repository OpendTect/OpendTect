#ifndef uistoredattrreplacer_h
#define uistoredattrreplacer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		June 2008
 RCS:		$Id: uistoredattrreplacer.h,v 1.7 2009-11-02 12:00:43 cvssatyaki Exp $
________________________________________________________________________

-*/
#include "attribdescid.h"
#include "linekey.h"
#include "sets.h"

class uiParent;
class BufferStringSet;
namespace Attrib
{
    class Desc;
    class DescSet;
};

mClass uiStoredAttribReplacer
{
public:
    				uiStoredAttribReplacer(uiParent*,
						       Attrib::DescSet&);
    void 			go();

protected:

    struct StoredEntry
    {
				StoredEntry( Attrib::DescID id1, LineKey lk )
				    : firstid_(id1)
				    , secondid_(Attrib::DescID::undef())
				    , lk_(lk) {}

	bool			operator == ( const StoredEntry& a ) const
	    			{ return firstid_ == a.firstid_
				      && secondid_ == a.secondid_
				      && lk_ == a.lk_; }

	bool			has2Ids() const
				{ return firstid_.isValid() &&
				    	 secondid_.isValid(); }
	Attrib::DescID		firstid_;
	Attrib::DescID		secondid_;
	LineKey			lk_;
    };

    void			getUserRef(const Attrib::DescID&,
					   BufferStringSet&) const;
    void			getStoredIds();
    void			handleSingleInput();
    void			handleMultiInput();
    bool			hasInput(const Attrib::Desc&,
					 const Attrib::DescID&) const;
    Attrib::DescSet& 		attrset_;
    TypeSet<StoredEntry>	storedids_;
    bool		 	is2d_;
    uiParent*	 		parent_;
    int				noofsteer_;
    int				noofseis_;
    bool			multiinpcube_;
};

#endif

