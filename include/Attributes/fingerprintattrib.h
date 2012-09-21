#ifndef fingerprintattrib_h
#define fingerprintattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          23-02-2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

/*!\brief FingerPrint Attribute

FingerPrint vector= nrattrib= 

Calculates the match with a definite vector.

Input:
0		Data 0
1		Data 1
.		.
.		.
nrattrib-1	Data nrattrib-1


Output:
0       Match

*/

namespace Attrib
{

mClass(Attributes) FingerPrint : public Provider
{
public:
    static void			initClass();
				FingerPrint(Desc&);

    static const char*		attribName()	{ return "FingerPrint"; }
    static const char*		refposStr()	{ return "refpos"; }
    static const char*		refposzStr()	{ return "refposz"; }
    static const char*		reflinesetStr()	{ return "reflineset"; }
    static const char*		ref2dlineStr()	{ return "ref2dline"; }
    static const char*		valStr()	{ return "value"; }
    static const char*		rangeStr()	{ return "range"; }
    static const char*		weightStr()	{ return "weight"; }
    static const char*		valpicksetStr()	{ return "valpickset"; }
    static const char*		statstypeStr()	{ return "statstype"; }
    static const char*		valreftypeStr()	{ return "valreftype"; }
    static const char*		rgpicksetStr()	{ return "rgpickset"; }
    static const char*		rgreftypeStr()	{ return "rgreftype"; }

    virtual bool		usesTracePosition() const;

protected:
    				~FingerPrint() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return false; }

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    TypeSet<float>		refvector_;
    TypeSet< Interval<float> >	ranges_;
    TypeSet<int>		weights_;
    
    TypeSet<float>		scaledref_;
    
    TypeSet<int>		dataidx_;
    ObjectSet<const DataHolder>	inputdata_;
};

} // namespace Attrib


#endif

