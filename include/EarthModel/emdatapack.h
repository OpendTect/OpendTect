#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2008
________________________________________________________________________

-*/

#include "emcommon.h"
#include "datapackbase.h"

template <class T> class Array2D;
class BinnedValueSet;
class DataPointSet;

namespace EM
{

class Horizon;
class Fault;


/*!\brief base class for Earth Model data packs. */

mExpClass(EarthModel) DataPackCommon : public ::FlatDataPack
{
public:
			DataPackCommon(const Object&,Array2D<float>*);
			DataPackCommon(const Object&,
				       const ObjectSet<BinnedValueSet>&);
			DataPackCommon(const Object&,const DataPointSet&);

    virtual const char*	sourceType() const	= 0;
    virtual bool	isVertical() const	= 0;

    const Object&	getEMObj() const	{ return emobj_; }

    void		dumpInfo(IOPar&) const;

protected:

    const Object&	emobj_;

};


/*!\brief Flat data pack class for Horizons. */

mExpClass(EarthModel) HorDataPack : public DataPackCommon
{
public:
			HorDataPack(const EM::Horizon&,Array2D<float>*);
			HorDataPack(const EM::Horizon&,
				    const ObjectSet<BinnedValueSet>&);
			HorDataPack(const EM::Horizon&,const DataPointSet&);

    virtual const char*	sourceType() const		{ return "Horizon"; }
    virtual bool	isVertical() const		{ return false;}

    Coord3		getCoord(int,int) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;

    const char*		dimName(bool) const;

};


/*!\brief Flat data pack from attribute extraction on faults. */

mExpClass(EarthModel) FaultDataPack : public DataPackCommon
{
public:

			FaultDataPack(const EM::Fault&,Array2D<float>*);
			FaultDataPack(const EM::Fault&,
				      const ObjectSet<BinnedValueSet>&);
			FaultDataPack(const EM::Fault&,const DataPointSet&);

    virtual const char*	sourceType() const		{ return "Fault"; }
    virtual bool	isVertical() const		{ return true; }

    const char*		dimName(bool) const;

    Coord3		getCoord(int,int) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;

};

} // namespace EM
