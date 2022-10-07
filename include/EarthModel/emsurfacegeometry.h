#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "bufstringset.h"
#include "emposid.h"
#include "emobject.h"
#include "mathfunc.h"
#include "rowcolsurface.h"

template <class T, class AT> class TopList;


class RowCol;
class ZAxisTransform;


namespace Geometry { class RowColSurface; }


namespace EM
{
class Surface;
class SurfaceIODataSelection;

/*!
\brief Surface geometry
*/

mExpClass(EarthModel) SurfaceGeometry : public CallBacker
{ mODTextTranslationClass(SurfaceGeometry);
public:
    virtual		~SurfaceGeometry();
    virtual void	removeAll();

    virtual bool	enableChecks(bool yn);
    virtual bool	isChecksEnabled() const;
    virtual bool	isNodeOK(const PosID&) const;

    virtual const Geometry::Element*	geometryElement() const;
    virtual Geometry::Element*		geometryElement();

    virtual int		getConnectedPos(const PosID& posid,
					TypeSet<PosID>* res) const;
			/*!<\returns the number of connected pos. */
    virtual void	getLinkedPos(const PosID& posid,TypeSet<PosID>&) const;
    virtual bool	isAtEdge(const PosID&) const;

    bool		isChanged(int) const		{ return changed_; }
    void		resetChangedFlag()		{ changed_=false; }

    virtual bool	isLoaded() const;
    virtual bool	isFullResolution() const;
    virtual Executor*	loader(const SurfaceIODataSelection* =nullptr,
				const ZAxisTransform* =nullptr);
    virtual Executor*	saver(const SurfaceIODataSelection* =nullptr,
			      const MultiID* =nullptr);

    mDeprecatedDef
    virtual int		findPos(const SectionID&,const Interval<float>& x,
				const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    virtual int		findPos(const Interval<float>& x,
				const Interval<float>& y,
				const Interval<float>& z,TypeSet<PosID>*) const;
    virtual int		findPos(const TrcKeyZSampling&,TypeSet<PosID>*) const;

    virtual EMObjectIterator*	createIterator(const TrcKeyZSampling* =0) const
				{ return nullptr; }

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

// Deprecated public functions
    mDeprecated("Use createIterator(const TrcKeyZSampling*)")
    EMObjectIterator*		createIterator(const EM::SectionID&,
					       const TrcKeyZSampling* t=0) const
				{ return createIterator(t); }
//    mDeprecatedObs
    int			nrSections() const;
    mDeprecatedObs
    SectionID		sectionID(int idx) const;
    mDeprecatedObs
    SectionID		sectionID(const char*) const;
    mDeprecatedObs
    bool		hasSection(const SectionID&) const;
    mDeprecatedObs
    int			sectionIndex(const SectionID&) const;
    mDeprecatedObs
    const char*		sectionName(const SectionID&) const;
    mDeprecatedObs
    bool		setSectionName(const SectionID&,const char*,
					bool addtohistory );
    mDeprecatedObs
    SectionID		addSection(const char* nm,bool addtohistory);
    mDeprecatedObs
    SectionID		addSection(const char* nm,const SectionID&,
				   bool addtohistory);
			/*!<Return false if the sectionid allready exists */
    mDeprecatedObs
    virtual bool	removeSection(const SectionID&,bool addtohistory);
    mDeprecatedObs
    virtual SectionID	cloneSection(const SectionID&);

    mDeprecated("Use geometryElement() const")
    virtual const Geometry::Element*	sectionGeometry(const SectionID&) const;
    mDeprecated("Use geometryElement()")
    virtual Geometry::Element*		sectionGeometry(const SectionID&);

protected:
				SurfaceGeometry(Surface&);

    mDeprecatedObs
    SectionID			addSectionInternal(Geometry::Element*,
					   const char* nm,const SectionID&,
					   bool addtohistory);


    virtual Geometry::Element*		createGeometryElement() const = 0;
    void				geomChangeCB(CallBacker*);

    Surface&				surface_;
    ObjectSet<Geometry::Element>	sections_;

    bool				changed_;
};


/*!
\brief RowCol SurfaceGeometry
*/

mExpClass(EarthModel) RowColSurfaceGeometry : public SurfaceGeometry
{ mODTextTranslationClass(RowColSurfaceGeometry);
public:
			RowColSurfaceGeometry(Surface&);
    virtual		~RowColSurfaceGeometry();


    const Geometry::RowColSurface* geometryElement() const override;
    Geometry::RowColSurface*	geometryElement() override;

    StepInterval<int>		rowRange() const;
    StepInterval<int>		colRange() const;
    StepInterval<int>		colRange(int row) const;

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =nullptr) const override;

// Deprecated public functions
    mDeprecated("Use geometryElement() const")
    const Geometry::RowColSurface* sectionGeometry(
					const SectionID&) const override
				{ return geometryElement(); }
    mDeprecated("Use geometryElement()")
    Geometry::RowColSurface*	sectionGeometry(const SectionID&) override
				{ return geometryElement(); }

    mDeprecated("Use without SectionID")
    StepInterval<int>		rowRange(const SectionID&) const;
    mDeprecated("Use without SectionID")
    StepInterval<int>		colRange(const SectionID&,int row) const
				{ return colRange(row); }
};

} // namespace EM
