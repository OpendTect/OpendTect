#ifndef segyhdrcalc_h
#define segyhdrcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2011
 RCS:		$Id: segyhdrcalc.h,v 1.3 2011-03-04 14:40:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "segyhdrdef.h"
#include "typeset.h"
class MathExpression;

 
namespace SEGY
{

mClass HdrCalc
{
public:

			HdrCalc( const HdrEntry& he, const char* def )
			    : he_(he), def_(def)	{}

    const HdrEntry&	he_;	//!< intended target
    BufferString	def_;	//!< user-defined math formula

};


mClass HdrCalcSet : public ObjectSet<HdrCalc>
{
public:
				HdrCalcSet(const HdrDef&);
				~HdrCalcSet();
    const HdrDef&		hdrDef()	{ return hdef_; }
    const HdrEntry&		trcIdxEntry()	{ return trcidxhe_; }

    int				indexOf(const char* entrynm) const;
    bool			add(const HdrEntry&,const char* def,
	    			    BufferString* emsg=0);
    void			discard(int);

    void			reSetSeqNr( int seqnr=1 ) { seqnr_ = seqnr; }
    void			apply(void*) const;

protected:

    const HdrDef&		hdef_;
    HdrEntry			trcidxhe_;
    ObjectSet<MathExpression>	exprs_;
    ObjectSet< TypeSet<int> >	heidxs_;
    mutable int			seqnr_;

};

} // namespace

#endif
