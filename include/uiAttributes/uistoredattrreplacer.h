#ifndef uistoredattrreplacer_h
#define uistoredattrreplacer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		June 2008
 RCS:		$Id: uistoredattrreplacer.h,v 1.10 2012-07-02 13:26:38 cvshelene Exp $
________________________________________________________________________

-*/
#include "attribdescid.h"
#include "bufstringset.h"
#include "linekey.h"
#include "sets.h"

class uiParent;
namespace Attrib
{
    class Desc;
    class DescSet;
};

mClass uiStoredAttribReplacer
{
public:
    				uiStoredAttribReplacer(uiParent*,
						       Attrib::DescSet*);
    				uiStoredAttribReplacer(uiParent*,IOPar*,
						       bool is2d=false);
    void 			go();

protected:

    struct StoredEntry
    {
				StoredEntry( Attrib::DescID id1, LineKey lk,
				       	     BufferString storedref )
				    : firstid_(id1)
				    , secondid_(Attrib::DescID::undef())
				    , lk_(lk)
       				    , storedref_(storedref)	{}

	bool			operator == ( const StoredEntry& a ) const
	    			{ return firstid_ == a.firstid_
				      && secondid_ == a.secondid_
				      && lk_ == a.lk_
				      && storedref_ == a.storedref_; }

	bool			has2Ids() const
				{ return firstid_.isValid() &&
				    	 secondid_.isValid(); }
	Attrib::DescID		firstid_;
	Attrib::DescID		secondid_;
	LineKey			lk_;
	BufferStringSet		userrefs_;
	BufferString		storedref_;
    };

    void			usePar(const IOPar&);
    void			setStoredKey(IOPar*,const char*);
    void			setSteerPar(StoredEntry,const char*,
	    				    const char*);
    void			setUserRef(IOPar*,const char*);
    void			getUserRefs(const IOPar&);
    void			getUserRef(const Attrib::DescID&,
					   BufferStringSet&) const;
    void			getStoredIds();
    void			getStoredIds(const IOPar&);
    void			handleSingleInput();
    void			handleMultiInput();
    bool			hasInput(const Attrib::Desc&,
					 const Attrib::DescID&) const;
    int				getOutPut(int descid);
    void			removeDescsWithBlankInp(const Attrib::DescID&);
    Attrib::DescSet* 		attrset_;
    IOPar*			iopar_;
    TypeSet<StoredEntry>	storedids_;
    bool		 	is2d_;
    uiParent*	 		parent_;
    int				noofsteer_;
    int				noofseis_;
    bool			multiinpcube_;
};

#endif

