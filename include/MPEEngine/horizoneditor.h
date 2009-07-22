#ifndef horizoneditor_h
#define horizoneditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: horizoneditor.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

#include "bufstringset.h"

namespace EM { class Horizon3D; };

namespace MPE
{

mClass HorizonEditor : public ObjectEditor
{
public:
    				HorizonEditor(EM::Horizon3D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    
    void			getEditIDs(TypeSet<EM::PosID>&) const;
    bool			addEditID( const EM::PosID& );
    bool			removeEditID( const EM::PosID& );

    bool			boxEditArea() const { return horbox; }
    void			setBoxEditArea( bool nb ) { horbox=nb; }

    const RowCol&		getEditArea() const { return editarea; }
    void			setEditArea( const RowCol& rc ) { editarea=rc; }

    const BufferStringSet*	getAlongMovingStyleNames() const;
    int				getAlongMovingStyle() const { return vertstyle;}
    void			setAlongMovingStyle( int nv ) { vertstyle=nv; }

protected:
    virtual			~HorizonEditor();
    void			getAlongMovingNodes( const EM::PosID&,
	    					     TypeSet<EM::PosID>&,
	                                             TypeSet<float>*) const;
    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    void			emChangeCB( CallBacker* );

    BufferStringSet		vertstylenames;

    RowCol			editarea;
    bool			horbox;
    int				vertstyle;
};


}; // namespace MPE

#endif

