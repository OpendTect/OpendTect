#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C Glas
 Date:		November 2008
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emfault.h"

namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }

namespace EM
{
class EMManager;

/*!
\brief FaultStickSet geometry.
*/

mExpClass(EarthModel) FaultStickSetGeometry : public FaultGeometry
{
public:
			FaultStickSetGeometry(Surface&);
			~FaultStickSetGeometry();

    int			nrSticks(const SectionID&) const;
    int			nrKnots(const SectionID&,int sticknr) const;

    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override;
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const MultiID* pickedmid,
				    const char* pickednm,bool addtohistory);
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    Pos::GeomID pickedgeomid,bool addtohistory);
    bool		removeStick(const SectionID&,int sticknr,
				    bool addtohistory) override;
    bool		insertKnot(const SectionID&,const SubID&,
			       const Coord3& pos,bool addtohistory) override;
    bool		removeKnot(const SectionID&,const SubID&,
				   bool addtohistory) override;

    bool		pickedOnPlane(const SectionID&,int sticknr) const;
    bool		pickedOn2DLine(const SectionID&,int sticknr) const;
    bool		pickedOnHorizon(const SectionID&,int sticknr) const;

    const MultiID*	pickedMultiID(const SectionID&,
				      int sticknr) const override;
    const char*		pickedName(const SectionID&,int sticknr) const override;
    Pos::GeomID		pickedGeomID(const SectionID&,int sticknr)const;

    Geometry::FaultStickSet*
			sectionGeometry(const SectionID&) override;
    const Geometry::FaultStickSet*
			sectionGeometry(const SectionID&) const override;

    EMObjectIterator*	createIterator(const SectionID&,
				   const TrcKeyZSampling* =0) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    Geometry::FaultStickSet*	createSectionGeometry() const override;

    int			indexOf(const SectionID& sid,int stricnr) const;

    struct StickInfo
    {
				StickInfo();

	int			sid;
	int			sticknr;
	Pos::GeomID		pickedgeomid;
	MultiID			pickedmid;
	BufferString		pickednm;
    };

    ObjectSet<StickInfo>	 stickinfo_;

};


/*!
\brief Fault stick set.
*/

mExpClass(EarthModel) FaultStickSet: public Fault
{ mDefineEMObjFuncs( FaultStickSet );
public:
    FaultStickSetGeometry&		geometry() override;
    const FaultStickSetGeometry&	geometry() const override;
    void				apply(const Pos::Filter&) override;
    uiString				getUserTypeStr() const override;


protected:

    const IOObjContext&			getIOObjContext() const override;

    FaultStickSetGeometry		geometry_;
};

} // namespace EM

