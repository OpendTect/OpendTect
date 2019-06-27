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

mExpClass(EarthModel) FaultSet3D : public EM::Object
{ mDefineEMObjFuncs( FaultSet3D );
public:

    uiString			getUserTypeStr() const;

    int				nrFaults() const;
    FaultID			getFaultID(int idx) const;

    FaultID			addFault(RefMan<Fault3D>);
    bool			addFault(RefMan<Fault3D>,FaultID);
    bool			removeFault(FaultID);


    RefMan<Fault3D>		getFault3D(FaultID);
    ConstRefMan<Fault3D>	getFault3D(FaultID) const;

    int				nrSections() const	{ return 1; }
    SectionID			sectionID(int) const	{ return 0; }

    Executor*			loader();
    virtual Executor*		saver();

protected:

    const IOObjContext&		getIOObjContext() const;
    int				indexOf(FaultID) const;

    ObjectSet<Fault3D>		faults_;
    TypeSet<FaultID>		ids_;
    int				curidnr_;
};

} // namespace EM
