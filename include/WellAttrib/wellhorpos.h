#ifndef wellhorpos_h
#define wellhorpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
 RCS:           $Id: wellhorpos.h,v 1.3 2010-08-13 12:31:12 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "emposid.h"

/*!brief used to give well info at horizon intersection. !*/

namespace Well { class Track; }
class BinID;
class BinIDValueSet;
class MultiID;

mClass WellHorPos
{
public:
    				WellHorPos(const Well::Track&,
					   const EM::ObjectID&);	

    void			intersectWellHor(BinIDValueSet&) const;
    				//in principle, only one position per well but 
    				//there may be more than one intersection (faults)
    void			intersectBinIDsHor(BinIDValueSet&) const;
    void			intersectBinIDHor(const BinID&,float&) const;

protected:

    const EM::ObjectID& 	horid_;
    const Well::Track&		track_;
    TypeSet<BinID>		wellbids_;

    void 			transformWellCoordsToBinIDs();
    void			setBidsFromHorType(BinID&) const;

};

#endif
