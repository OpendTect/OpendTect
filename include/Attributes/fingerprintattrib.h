#ifndef fingerprintattrib_h
#define fingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          23-02-2006
 RCS:           $Id: fingerprintattrib.h,v 1.1 2006-04-18 11:09:05 cvshelene Exp $
________________________________________________________________________

-*/

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

class FingerPrint : public Provider
{
public:
    static void			initClass();
				FingerPrint(Desc&);

    static const char*		attribName()	{ return "FingerPrint"; }
    static const char*		refposStr()	{ return "refpos"; }
    static const char*		refposzStr()	{ return "refposz"; }
    static const char*		valStr()	{ return "value"; }

protected:
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return false; }

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples) const;

    TypeSet<float>		vector_;
    BinID			refpos_;
    float			refposz_;

    TypeSet<int>		dataidx_;
    ObjectSet<const DataHolder>	inputdata_;
};

} // namespace Attrib


#endif
