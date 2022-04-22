#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sep 2018
________________________________________________________________________


-*/

#include "emfault3d.h"

namespace EM
{

typedef int FaultID;


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

    int				nrSections() const override	{ return 1; }
    SectionID			sectionID(int) const override	{ return 0; }

    Executor*			loader() override;
    virtual Executor*		saver() override;

protected:

    const IOObjContext&		getIOObjContext() const override;
    int				indexOf(FaultID) const;

    friend class		ObjectManager;

    ObjectSet<Fault3D>		faults_;
    TypeSet<FaultID>		ids_;
    int				curidnr_;
};

} // namespace EM
