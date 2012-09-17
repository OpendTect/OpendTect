#ifndef horizon3dextender_h
#define horizon3dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.10 2011/05/11 07:17:04 cvsumesh Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon3D; };

namespace MPE
{

mClass BaseHorizon3DExtender : public SectionExtender
{
public:
    //static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    //static void			initClass();

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const CubeSampling&		getExtBoundary() const;

protected:
    				BaseHorizon3DExtender(EM::Horizon3D& surface_,
						const EM::SectionID& sid);

    virtual float		getDepth(const BinID& src,
					 const BinID& dest) const;
    //virtual void		prepareDataIfRequired() { return; }

    BinIDValue			direction;
    EM::Horizon3D&		surface;
};


mClass Horizon3DExtender : public BaseHorizon3DExtender
{
public:
    static void			initClass();
    static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    				Horizon3DExtender(EM::Horizon3D& surface_,
						  const EM::SectionID& sid);
};


}; // namespace MPE

#endif

