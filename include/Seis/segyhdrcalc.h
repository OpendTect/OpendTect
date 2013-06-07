#ifndef segyhdrcalc_h
#define segyhdrcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "segyhdrdef.h"
#include "typeset.h"
class Executor;
class MathExpression;
class BufferStringSet;

 
namespace SEGY
{
class BinHeader;
class TxtHeader;

mClass HdrCalc
{
public:

			HdrCalc( const HdrEntry& he, const char* def )
			    : he_(he), def_(def)	{}

    const HdrEntry&	he_;	//!< intended target
    BufferString	def_;	//!< user-defined math formula

    inline BufferString	getDispStr() const
			{ return BufferString( he_.name(), " = ", def_ ); }

};


mClass HdrCalcSet : public ObjectSet<HdrCalc>
		  , public NamedObject
{
public:
				HdrCalcSet(const HdrDef&);
				~HdrCalcSet();
    const HdrDef&		hdrDef()	{ return hdef_; }
    const HdrEntry&		trcIdxEntry()	{ return trcidxhe_; }

    int				indexOf(const char* entrynm) const;
    int				indexOf( const HdrCalc* he ) const
				{ return ObjectSet<HdrCalc>::indexOf(he); }
    bool			set(int,const char* def,BufferString* emsg=0);
    bool			add(const HdrEntry&,const char* def,
	    			    BufferString* emsg=0);
    bool			add(const char* dispstr);
    void			discard(int);
    void			setEmpty();

    void			reSetSeqNr( int seqnr=1 ) { seqnr_ = seqnr; }
    void			apply(void*,bool needswap) const;
    Executor*			getApplier(std::istream&,std::ostream&,
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
    ObjectSet<MathExpression>	exprs_;
    ObjectSet< TypeSet<int> >	heidxs_;
    mutable int			seqnr_;

    MathExpression*		gtME(const char*,TypeSet<int>&,
	    			     BufferString*) const;

};

} // namespace

#endif
