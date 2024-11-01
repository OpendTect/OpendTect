#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emeditor.h"

namespace EM { class PolygonBody; };
template <class T> class Selector;

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::PolygonBody.
*/

mExpClass(MPEEngine) PolygonBodyEditor : public ObjectEditor
{
public:

    static RefMan<PolygonBodyEditor> create(const EM::PolygonBody&);

    void			setLastClicked(const EM::PosID&);
    void			setSowingPivot(const Coord3);
    bool			removeSelection(const Selector<Coord3>&);

    void			getInteractionInfo(EM::PosID& nearestpid0,
					   EM::PosID& nearestpid1,
					   EM::PosID& insertpid,
					   const Coord3&,float zfactor) const;

protected:
				~PolygonBodyEditor();

private:
				PolygonBodyEditor(const EM::PolygonBody&);


    bool			setPosition(const EM::PosID&,
					    const Coord3&) override;
    bool			setPosition( const Coord3& c ) override
				{ return ObjectEditor::setPosition(c); }
    Geometry::ElementEditor*	createEditor() override;
    float			getNearestPolygon(int& polygon,
						  const Coord3&,
						  float zfactor) const;
    void			getPidsOnPolygon(EM::PosID& nearestpid0,
						 EM::PosID& nearestpid1,
						 EM::PosID& insertpid,
						 int polygon,
						 const Coord3&,
						 float zfactor) const;

    Coord3			sowingpivot_;
    mutable TypeSet<Coord3>	sowinghistory_;

    static EM::PosID&		lastclickedpid();

};

}  // namespace MPE
