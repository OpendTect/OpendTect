#ifndef polygonsurfeditor_h
#define polygonsurfeditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          July 2008
 RCS:           $Id: polygonsurfeditor.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class PolygonBody; };
template <class T> class Selector;

namespace MPE
{

mClass PolygonBodyEditor : public ObjectEditor
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

