#ifndef segyhdrcalc_h
#define segyhdrcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2011
 RCS:		$Id: segyhdrcalc.h,v 1.1 2011-03-02 16:11:04 cvsbert Exp $
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

    void			add(HdrCalc*);
    void			discard(int);

    bool			getValues(const void* buf,TypeSet<int>&) const;

protected:

    const HdrDef&		hdef_;
    HdrEntry			seqnr_;
    ObjectSet<MathExpression>	exprs_;

};

} // namespace

#endif
