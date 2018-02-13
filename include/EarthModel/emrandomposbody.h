#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
________________________________________________________________________


-*/

#include "embody.h"

class DataPointSet;
namespace Pick { class Set; }

namespace EM
{

/*!\brief Random position Body. */

mExpClass(EarthModel) RandomPosBody : public Body, public Object
{   mDefineEMObjFuncs( RandomPosBody );
    mODTextTranslationClass( RandomPosBody );
public:

    const char*			type() const { return typeStr(); }

    Geometry::Element*		geometryElement() { return 0; }
    const Geometry::Element*	geometryElement() const
				{ return 0; }

    void			copyFrom(const Pick::Set&);//get my own picks.
    void			copyFrom(const DataPointSet&,int selgrp);
				//copy all for selgrp < 0.
    void			copyFrom(const DataPointSet&,int dpscolid,
					 const Interval<float>& valrg);
    void			setPositions(const TypeSet<Coord3>&);
    const TypeSet<Coord3>&	getPositions() const	{ return locations_; }
    bool			addPos(const Coord3&);

    const TypeSet<EM::PosID>&	posIDs() const		{ return ids_; }

    Coord3			getPos(const EM::PosID&) const;
    const IOObjContext&		getIOObjContext() const;
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*);
    virtual Executor*		loader();
    virtual bool		isEmpty() const;

    virtual ImplicitBody*	createImplicitBody(const TaskRunnerProvider&,
						   bool) const;
    bool			getBodyRange(TrcKeyZSampling&);

    DBKey			storageID() const;
    BufferString		storageName() const;

    void			refBody();
    void			unRefBody();

    bool			useBodyPar(const IOPar&);
    void			fillBodyPar(IOPar&) const;

    uiString			getUserTypeStr() const
				{ return tr("Random Position Body"); }

    static const char*		sKeyPosIDs()	{ return "Position IDs"; }
protected:

     virtual bool		setPosition(const EM::PosID&,
					    const Coord3&,bool addtohistory,
					    NodeSourceType tp=Auto);
    TypeSet<Coord3>		locations_;
    TypeSet<EM::PosID>		ids_;
};

} // namespace EM
