#ifndef emeditor_h
#define emeditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emeditor.h,v 1.2 2005-01-07 12:18:25 kristofer Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "emposid.h"

template <class T> class TypeSet;

namespace EM { class EMObject; };
namespace Geometry { class ElementEditor; };

namespace MPE
{

class ObjectEditor
{
public:
    			ObjectEditor( EM::EMObject& );
    const EM::EMObject&	emObject() const { return emobject; }
    virtual void	startEdit();
    virtual void	finishEdit();

    virtual void	getEditIDs( TypeSet<EM::PosID>& ) const;
    virtual Coord3	getPosition( const EM::PosID& ) const;
    virtual bool	setPosition( const EM::PosID&, const Coord3& );

    virtual bool	mayTranslate1D( const EM::PosID& ) const;
    virtual Coord3	translation1DDirection( const EM::PosID& ) const;

    virtual bool	mayTranslate2D( const EM::PosID& ) const;
    virtual Coord3	translation2DNormal( const EM::PosID& ) const;

    virtual bool	mayTranslate3D( const EM::PosID& ) const;

    virtual bool	maySetNormal( const EM::PosID& ) const;
    virtual Coord3	getNormal( const EM::PosID& ) const;

    virtual bool	maySetDirection( const EM::PosID& ) const;
    virtual Coord3	getDirectionPlaneNormal( const EM::PosID& ) const;
    virtual Coord3	getDirection( const EM::PosID& ) const;

protected:

    Geometry::ElementEditor*		getEditor( const EM::SectionID& );
    const Geometry::ElementEditor*	getEditor( const EM::SectionID& ) const;

    ObjectSet<Geometry::ElementEditor>	geeditors;
    EM::EMObject&			emobject;
    TypeSet<EM::PosID>			changedpids;
};


typedef ObjectEditor*(*EMEditorCreationFunc)(EM::EMObject*);


class EditorFactory
{
public:
			EditorFactory( const char* emtype,
				       EMEditorCreationFunc );
    const char*		emObjectType() const;
    ObjectEditor*	create( EM::EMObject* emobj = 0 ) const;

protected:
    EMEditorCreationFunc	createfunc;
    const char*			type;

};


};

#endif

