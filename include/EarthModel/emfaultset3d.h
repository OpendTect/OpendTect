#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfault3d.h"

namespace EM
{

/*!
\brief 3D FaultSet
*/

mExpClass(EarthModel) FaultSet3D : public EMObject
{ mDefineEMObjFuncs( FaultSet3D );
public:

    uiString			getUserTypeStr() const override;

    int				nrFaults() const;
    FaultID			getFaultID(int idx) const;

    FaultID			addFault(RefMan<Fault3D>);
    bool			addFault(RefMan<Fault3D>,FaultID);
    bool			removeFault(FaultID);


    RefMan<Fault3D>		getFault3D(FaultID);
    ConstRefMan<Fault3D>	getFault3D(FaultID) const;

    Executor*			loader() override;
    virtual Executor*		saver() override;

    EMObjectIterator*		createIterator(
				 const TrcKeyZSampling* =nullptr) const override
				{ return nullptr; }

protected:

    const IOObjContext&		getIOObjContext() const override;
    int				indexOf(FaultID) const;

    friend class		ObjectManager;

    ObjectSet<Fault3D>		faults_;
    TypeSet<FaultID>		ids_;
    int				curidnr_ = 0;
};

} // namespace EM
