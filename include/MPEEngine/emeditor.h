#ifndef emeditor_h
#define emeditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emeditor.h,v 1.6 2005-07-01 00:39:27 cvskris Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "emposid.h"

template <class T> class TypeSet;

namespace EM { class EMObject; };
namespace Geometry { class ElementEditor; };

namespace MPE
{

class ObjectEditor : public CallBackClass
{
public:
    				ObjectEditor( EM::EMObject& );
    				~ObjectEditor();
    const EM::EMObject&		emObject() const	{ return emobject; }
    virtual void		startEdit(const EM::PosID& =EM::PosID(-1,-1,-1));
    virtual bool		setPosition(const Coord3&);
    virtual void		finishEdit();

    virtual void		setEditIDs(const TypeSet<EM::PosID>&);
    virtual void		getEditIDs(TypeSet<EM::PosID>&) const;
    virtual Coord3		getPosition(const EM::PosID&) const;
    virtual bool		setPosition(const EM::PosID&,const Coord3&);

    virtual bool		mayTranslate1D(const EM::PosID&) const;
    virtual Coord3		translation1DDirection(const EM::PosID&) const;

    virtual bool		mayTranslate2D(const EM::PosID&) const;
    virtual Coord3		translation2DNormal(const EM::PosID&) const;

    virtual bool		mayTranslate3D(const EM::PosID&) const;

    virtual bool		maySetNormal(const EM::PosID&) const;
    virtual Coord3		getNormal(const EM::PosID&) const;

    virtual bool		maySetDirection(const EM::PosID&) const;
    virtual Coord3		getDirectionPlaneNormal(const EM::PosID&) const;
    virtual Coord3		getDirection(const EM::PosID&) const;

    Notifier<ObjectEditor>	editpositionchange;
    				/*!<Won't trigger on position-changes,
				    but when new edit positions are avaliable
				    or editpositions has been removed */

protected:

    Geometry::ElementEditor*		getEditor( const EM::SectionID& );
    const Geometry::ElementEditor*	getEditor( const EM::SectionID& ) const;
    virtual Geometry::ElementEditor*	createEditor( const EM::SectionID& )=0;

    void				editPosChangeTrigger(CallBacker*);
    void				emSectionChange(CallBacker*);

    virtual void			setAlongMovingNodes();
    TypeSet<EM::PosID>			alongmovingnodes;
    TypeSet<float>			alongmovingnodesfactors;
    TypeSet<Coord3>			alongmovingnodesstart;

    EM::EMObject&			emobject;
    EM::PosID				movingnode;
    Coord3				startpos;
    TypeSet<EM::PosID>			changedpids;

    TypeSet<EM::PosID>			editids;

private:
    ObjectSet<Geometry::ElementEditor>	geeditors;
    TypeSet<EM::SectionID>		sections;
};


typedef ObjectEditor*(*EMEditorCreationFunc)(EM::EMObject&);


class EditorFactory
{
public:
			EditorFactory( const char* emtype,
				       EMEditorCreationFunc );
    const char*		emObjectType() const;
    ObjectEditor*	create( EM::EMObject& ) const;

protected:
    EMEditorCreationFunc	createfunc;
    const char*			type;
};



};

#endif

