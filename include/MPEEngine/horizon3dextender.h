#ifndef horizon3dextender_h
#define horizon3dextender_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: horizon3dextender.h,v 1.9 2011-05-02 06:14:48 cvsumesh Exp $
________________________________________________________________________

-*/

#include "sectionextender.h"
#include "position.h"

namespace EM { class Horizon3D; };

namespace MPE
{

mClass Horizon3DExtender : public SectionExtender
{
public:
    				Horizon3DExtender(EM::Horizon3D& surface_,
						const EM::SectionID& sid);

    static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    static void			initClass();

    void			setDirection(const BinIDValue&);
    const BinIDValue*		getDirection() const { return &direction; }

    int				nextStep();

    int				maxNrPosInExtArea() const;
    void			preallocExtArea();

    const CubeSampling&		getExtBoundary() const;

protected:

    virtual const float		getDepth(const BinID& src,const BinID& dest);
    virtual void		prepareDataIfRequired() { return; }

    BinIDValue			direction;
    EM::Horizon3D&		surface;
};


mClass BaseHorizon3DExtender : public Horizon3DExtender
{
public:
    static void			initClass();
    static SectionExtender*	create(EM::EMObject*,const EM::SectionID&);
    				BaseHorizon3DExtender(EM::Horizon3D& surface_,
						      const EM::SectionID& sid);
};


}; // namespace MPE

#endif

