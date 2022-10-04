#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "posprovider.h"
#include "binidvalset.h"
#include "bufstring.h"
class IOObj;

namespace Pos
{

/*!\brief Provider based on BinIDValueSet table */

mExpClass(Geometry) TableProvider3D : public Provider3D
{
public:
			TableProvider3D();
			TableProvider3D(const IOObj& psioobj);
			TableProvider3D(const char* filenm);
			TableProvider3D(const TableProvider3D&);
			~TableProvider3D();

    TableProvider3D&	operator =(const TableProvider3D&);
    const char*		type() const override;	//!< sKey::Table()
    const char*		factoryKeyword() const override { return type(); }
    TableProvider3D*	clone() const override
			    { return new TableProvider3D(*this); }

    void		reset() override		{ pos_.reset(); }

    bool		toNextPos() override	{ return bvs_.next(pos_,true); }
    bool		toNextZ() override
			    { return bvs_.next(pos_,false); }

    BinID		curBinID() const override
			    { return bvs_.getBinID(pos_); }

    float		curZ() const override	{ return *bvs_.getVals(pos_); }
    bool		includes(const BinID&,float) const override;
    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getExtent(BinID&,BinID&) const override;
    void		getZRange(Interval<float>&) const override;
    od_int64		estNrPos() const override { return bvs_.totalSize(); }
    int			estNrZPerPos() const override	{ return 1; }

    BinIDValueSet&	binidValueSet()		{ return bvs_; }
    const BinIDValueSet& binidValueSet() const	{ return bvs_; }

    static void		getBVSFromPar(const IOPar&,BinIDValueSet&);

    bool		includes( const Coord& c, float z ) const override
			{ return Provider3D::includes(c,z); }

protected:

    BinIDValueSet	bvs_;
    BinIDValueSet::SPos pos_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new TableProvider3D; }

};

} // namespace Pos
