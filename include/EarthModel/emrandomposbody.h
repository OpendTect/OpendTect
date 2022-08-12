#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "embody.h"
#include "emobject.h"

class DataPointSet;
namespace Pick { class Set; }

namespace EM
{

/*!
\brief Random position Body.
*/

mExpClass(EarthModel) RandomPosBody : public Body, public EMObject
{ mDefineEMObjFuncs( RandomPosBody );
public:

    const char*			type() const override { return typeStr(); }

    void			copyFrom(const Pick::Set&);//get my own picks.
    void			copyFrom(const DataPointSet&,int selgrp);
				//copy all for selgrp < 0.
    void			copyFrom(const DataPointSet&,int dpscolid,
					 const Interval<float>& valrg);
    void			setPositions(const TypeSet<Coord3>&);
    const TypeSet<Coord3>&	getPositions() const	{ return locations_; }
    bool			addPos(const Coord3&);

    const TypeSet<EM::SubID>&	posIDs() const		{ return ids_; }

    Coord3			getPos(const PosID&) const override;
    Coord3			getPos(const SubID&) const override;
    bool			setPos(const PosID&,const Coord3&,
					bool addtohistory) override;
    bool			setPos(const SubID&,
				    const Coord3&,bool addtohistory) override;
    const IOObjContext&		getIOObjContext() const override;
    Executor*			saver() override;
    virtual Executor*		saver(IOObj*);
    Executor*			loader() override;
    bool			isEmpty() const override;

    ImplicitBody*		createImplicitBody(TaskRunner*,
						   bool) const override;
    bool			getBodyRange(TrcKeyZSampling&) override;

    MultiID			storageID() const override;
    BufferString		storageName() const override;

    void			refBody() override;
    void			unRefBody() override;

    bool			useBodyPar(const IOPar&) override;
    void			fillBodyPar(IOPar&) const override;

    uiString			getUserTypeStr() const override
				{ return tr("Random Position Body"); }

    static const char*		sKeySubIDs()	{ return "Position IDs"; }
protected:

    TypeSet<Coord3>		locations_;
    TypeSet<EM::SubID>		ids_;
};

} // namespace EM
