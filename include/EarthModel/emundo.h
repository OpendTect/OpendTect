#ifndef emundo_h
#define emundo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emundo.h,v 1.1 2007-07-06 14:11:05 cvskris Exp $
________________________________________________________________________


-*/

#include "undo.h"
#include "emposid.h"
#include "position.h"

class IOPar;
namespace Geometry { class ParametricSurface; };

namespace EM
{

class SetPosUndoEvent : public UndoEvent
{
public:
			SetPosUndoEvent( const Coord3& oldpos,
					    const EM::PosID& );

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    EM::PosID		posid;
    Coord3		savedpos;

    static const char*	savedposstr;
};


class SetPosAttribUndoEvent : public UndoEvent
{
public:
			SetPosAttribUndoEvent( const EM::PosID&,
						  int attrib, bool yn );

    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    EM::PosID		posid;
    bool		yn;
    int			attrib;
};


class SurfaceRelationEvent : public UndoEvent
{
public:
    			SurfaceRelationEvent( char prevrelation,
				const EM::ObjectID& cuttedsurface,
				const EM::SectionID& cuttedsection,
				const EM::ObjectID& cuttingsurface,
				const EM::SectionID& cuttingsection );
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    bool		restoreRelation();

    char		prevrelation;
    EM::ObjectID	cuttedobject;
    EM::SectionID	cuttedsection;
    EM::ObjectID	cuttingobject;
    EM::SectionID	cuttingsection;
};

/*! Saves information from a EMObject::changePosID call */

class PosIDChangeEvent : public UndoEvent
{
public:
    			PosIDChangeEvent( const EM::PosID& from,
					  const EM::PosID& to,
					  const Coord3& tosprevpos );
    const char*		getStandardDesc() const;
    bool		unDo();
    bool		reDo();

protected:
    const EM::PosID	from;
    const EM::PosID	to;
    Coord3		savedpos;
};


}; // Namespace


#endif
