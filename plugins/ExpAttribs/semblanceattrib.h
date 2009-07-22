#ifndef semblanceattrib_h
#define semblanceattrib_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: semblanceattrib.h,v 1.2 2009-07-22 16:01:26 cvsbert Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"

namespace Attrib
{

class Semblance : public Provider
{
public:

    static void			initClass();
				Semblance(Desc&);

    static const char*		attribName()    { return "Semblance"; }
    static const char*		inlszStr()	{ return "inlsize"; }
    static const char*		crlszStr()	{ return "crlsize"; }
    static const char*		zszStr()	{ return "zsize"; }

protected:
				~Semblance() {}
    static Provider*		createInstance(Desc&);

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;
    const BinID*		desStepout(int,int) const;
    const Interval<int>*	desZSampMargin(int,int) const;
    int				inlsz_;
    int				crlsz_;
    int				zsz_;
    BinID			stepout_;
    Interval<int>		zintv_;

    ObjectSet<const DataHolder> inpdata_;
    int				dataidx_;
};

} // namespace Attrib

#endif
