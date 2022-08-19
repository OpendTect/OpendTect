#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "attribdescid.h"
#include "bufstringset.h"
#include "datapack.h"
#include "linekey.h"
#include "sets.h"
#include "uistring.h"

class uiParent;
namespace Attrib
{
    class Desc;
    class DescSet;
};

mExpClass(uiAttributes) uiStoredAttribReplacer
{ mODTextTranslationClass(uiStoredAttribReplacer);
public:

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

				uiStoredAttribReplacer(uiParent*,
						       Attrib::DescSet*);
				uiStoredAttribReplacer(uiParent*,IOPar*,
						       bool is2d=false);
				~uiStoredAttribReplacer();
    void			go();
    void			setDataPackIDs(
				    const TypeSet<DataPack::FullID>&);

protected:

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
    TypeSet<DataPack::FullID>	dpfids_;
    bool		 	is2d_;
    uiParent*	 		parent_;
    int				noofsteer_;
    int				noofseis_;
    bool			multiinpcube_;
};
