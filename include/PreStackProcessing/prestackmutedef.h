#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "mathfunc.h"
#include "multiid.h"
#include "namedobj.h"
#include "odcommonenums.h"
#include "position.h"
#include "samplingdata.h"

class UnitOfMeasure;
namespace ZDomain { class Info; }


namespace PreStack
{

/*!
\brief NamedObject for definition of a mute function.
*/

mExpClass(PreStackProcessing) MuteDef : public NamedObject
{
public:
					MuteDef(const char* nm=nullptr);
					MuteDef(const MuteDef&);
					~MuteDef();

    MuteDef&				operator=(const MuteDef&);

    int					size() const;
    int					indexOf(const BinID&) const;
    PointBasedMathFunction&		getFn(int idx);
    BinID&				getPos(int idx);
    const PointBasedMathFunction&	getFn(int idx) const;
    const BinID&			getPos(int idx) const;

    void				fillPar(IOPar&) const;
    bool				usePar(const IOPar&);

    void				add(PointBasedMathFunction*,
					    const BinID& pos);
					//!<Function becomes mine.
    void				remove(int idx);
    float				value(float offs,const BinID&) const;
					//!< Interpolates between defined
					//!< positions
    void				computeIntervals(float offs,
					    const BinID&,
					    TypeSet<Interval<float> >&) const;
					/*!<Interpolates between
					  defined positions. */

    bool				isChanged() const { return ischanged_; }
    void				setChanged(bool yn) { ischanged_=yn; }

    bool				isOffsetInMeters() const;
    bool				isOffsetInFeet() const;
    Seis::OffsetType			offsetType() const;
    const ZDomain::Info&		zDomain() const;
    bool				zIsTime() const;
    bool				zInMeter() const;
    bool				zInFeet() const;
    MuteDef&				setOffsetType(Seis::OffsetType);
    MuteDef&				setZDomain(const ZDomain::Info&);
    const UnitOfMeasure*		zUnit() const;
    const UnitOfMeasure*		offsetUnit() const;

    void				setReferenceHorizon(const MultiID&);
    const MultiID&			getReferenceHorizon() const;

    static const char*			sKeyRefHor();

protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
    MultiID				refhor_;

    bool				ischanged_ = false;

    void				getAllZVals(TypeSet<float>&) const;
};

} // namespace PreStack
