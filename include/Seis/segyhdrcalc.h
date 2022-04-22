#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "seismod.h"
#include "namedobj.h"
#include "segyhdrdef.h"
#include "typeset.h"
#include "od_iosfwd.h"
#include "uistring.h"
class Executor;
class BufferStringSet;
namespace Math { class Expression; }


namespace SEGY
{
class BinHeader;
class TxtHeader;

mExpClass(Seis) HdrCalc
{ mODTextTranslationClass(HdrCalc);
public:

			HdrCalc( const HdrEntry& he, const char* def )
			    : he_(he), def_(def)	{}

    const HdrEntry&	he_;	//!< intended target
    BufferString	def_;	//!< user-defined math formula

    inline BufferString	getDispStr() const
			{ return BufferString( he_.name(), " = ", def_ ); }

};


mExpClass(Seis) HdrCalcSet : public ObjectSet<HdrCalc>
		  , public NamedObject
{ mODTextTranslationClass(HdrCalcSet);
public:
				HdrCalcSet(const HdrDef&);
				~HdrCalcSet();
    const HdrDef&		hdrDef()	{ return hdef_; }
    const HdrEntry&		trcIdxEntry()	{ return trcidxhe_; }

    int				indexOf(const char* entrynm) const;
    int				indexOf( const HdrCalc* he ) const override
				{ return ObjectSet<HdrCalc>::indexOf(he); }
    bool			set(int,const char* def,uiString* emsg=0);
    bool			add(const HdrEntry&,const char* def,
				    uiString* emsg=0);
    bool			add(const char* dispstr);
    void			discard(int);
    void			setEmpty();

    void			reSetSeqNr( int seqnr=1 ) { seqnr_ = seqnr; }
    void			apply(void*,bool needswap) const;
    Executor*			getApplier(od_istream&,od_ostream&,
					   int data_bytes_per_trace,
					   const BinHeader* bh=0,
					   const TxtHeader* th=0) const;

    bool			storeInSettings() const;
    void			getFromSettings(const char*);

    static void			getStoredNames(BufferStringSet&);
    static const char*		sKeySettsFile()	{ return "segyhdrcalc"; }

protected:

    const HdrDef&		hdef_;
    HdrEntry			trcidxhe_;
    ObjectSet<Math::Expression>	exprs_;
    ObjectSet< TypeSet<int> >	heidxs_;
    mutable int			seqnr_;

    Math::Expression*		gtME(const char*,TypeSet<int>&,
				     uiString*) const;

};

} // namespace

