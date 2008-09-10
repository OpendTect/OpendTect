#ifndef polygonsurfeditor_h
#define polygonsurfeditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          July 2008
 RCS:           $Id: polygonsurfeditor.h,v 1.3 2008-09-10 21:36:50 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class PolygonBody; };
template <class T> class Selector;

namespace MPE
{

class PolygonBodyEditor : public ObjectEditor
{
public:
    				PolygonBodyEditor(EM::PolygonBody&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();
    void			getInteractionInfo(EM::PosID& nearestpid0,
					   EM::PosID& nearestpid1,
					   EM::PosID& insertpid,
					   const Coord3&,float zfactor) const;
    bool			removeSelection(const Selector<Coord3>&);

protected:

    bool			setPosition(const EM::PosID&,const Coord3&);
    Geometry::ElementEditor* 	createEditor(const EM::SectionID&);
    float			getNearestPolygon(int& polygon,
	    					  EM::SectionID& sid,
						  const Coord3&,
						  float zfactor) const;
    void			getPidsOnPolygon(EM::PosID& nearestpid0,
						 EM::PosID& nearestpid1, 
       						 EM::PosID& insertpid,
       						 int polygon,
						 const EM::SectionID&,
       						 const Coord3&,
						 float zfactor) const;
};


};  // namespace MPE

#endif

