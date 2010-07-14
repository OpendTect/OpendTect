#ifndef wellhorpos_h
#define wellhorpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
 RCS:           $Id: wellhorpos.h,v 1.1 2010-07-14 13:58:28 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

/*!brief used to give well info at horizon intersection. !*/

namespace Well { class Track; }
namespace EM   { class Horizon; }
class BinID;
class BinIDValueSet;

mClass WellHorPos
{
public:
    				WellHorPos(const Well::Track&,const EM::Horizon&);		

    void			intersectWellHor(BinIDValueSet&) const;
    				//in principle, only one position per well but 
    				//there may be more than one intersection (faults)
    void			intersectBinIDsHor(BinIDValueSet&) const;
    void			intersectBinIDHor(const BinID&,float&) const;

protected:

    const EM::Horizon&		horizon_;
    const Well::Track&		track_;
    TypeSet<BinID>		wellbids_;

    void 			transformWellCoordsToBinIDs();
    void			setBidsFromHorType(BinIDValueSet&) const;

};

#endif
