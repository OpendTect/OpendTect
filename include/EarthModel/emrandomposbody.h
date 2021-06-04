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

    const char*			type() const { return typeStr(); }
    virtual int			nrSections() const		{ return 1; }
    virtual SectionID		sectionID(int) const		{ return 0; }
    virtual bool		canSetSectionName() const	{ return 0; }

    Geometry::Element*		sectionGeometry(const SectionID&) { return 0; }
    const Geometry::Element*	sectionGeometry(const SectionID&) const
				{ return 0; }

    void			copyFrom(const Pick::Set&);//get my own picks.
    void			copyFrom(const DataPointSet&,int selgrp);
				//copy all for selgrp < 0.
    void			copyFrom(const DataPointSet&,int dpscolid,
					 const Interval<float>& valrg);
    void			setPositions(const TypeSet<Coord3>&);
    const TypeSet<Coord3>&	getPositions() const	{ return locations_; }
    bool			addPos(const Coord3&);

    const TypeSet<EM::SubID>&	posIDs() const		{ return ids_; }

    Coord3			getPos(const EM::PosID&) const;
    Coord3			getPos(const EM::SectionID&,
					const EM::SubID&) const;
    bool			setPos(const EM::PosID&,const Coord3&,
					bool addtohistory);
    bool			setPos(const EM::SectionID&,const EM::SubID&,
					const Coord3&,bool addtohistory);
    const IOObjContext&		getIOObjContext() const;
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*);
    virtual Executor*		loader();
    virtual bool		isEmpty() const;

    ImplicitBody*		createImplicitBody(TaskRunner*,bool) const;
    bool			getBodyRange(TrcKeyZSampling&);

    MultiID			storageID() const;
    BufferString		storageName() const;

    void			refBody();
    void			unRefBody();

    bool			useBodyPar(const IOPar&);
    void			fillBodyPar(IOPar&) const;

    uiString			getUserTypeStr() const
				{ return tr("Random Position Body"); }

    static const char*		sKeySubIDs()	{ return "Position IDs"; }
protected:

    TypeSet<Coord3>		locations_;
    TypeSet<EM::SubID>		ids_;
};

} // namespace EM

