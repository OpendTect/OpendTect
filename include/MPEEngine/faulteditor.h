#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
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

protected:

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
};


};  // namespace MPE

#endif

