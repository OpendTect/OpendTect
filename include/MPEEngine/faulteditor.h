#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.2 2008-03-26 13:53:54 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class Fault; };

namespace MPE
{

class FaultEditor : public ObjectEditor
{
public:
    				FaultEditor(EM::Fault&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			getEditIDs(TypeSet<EM::PosID>&) const;
    bool			addEditID( const EM::PosID& );
    bool			removeEditID( const EM::PosID& );

protected:

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    TypeSet<EM::PosID>		editpids_;
};


};  // namespace MPE

#endif

