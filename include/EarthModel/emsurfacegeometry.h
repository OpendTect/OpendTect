#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emobject.h"
#include "mathfunc.h"
#include "rowcolsurface.h"

template <class T, class AT> class TopList;


class RowCol;


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
			SurfaceGeometry(Surface&);
    virtual		~SurfaceGeometry();

    SurfaceGeometry&	operator=(const SurfaceGeometry&);

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
    virtual Executor*	loader(const SurfaceIODataSelection* s=0);
    virtual Executor*	saver(const SurfaceIODataSelection* s=0,
			      const DBKey* key=0);

    virtual int		findPos(const Interval<float>& x,
				const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    virtual int		findPos(const TrcKeyZSampling&,TypeSet<PosID>*) const;

    virtual ObjectIterator*	createIterator(const TrcKeyZSampling* =0) const;


    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

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


    const Geometry::RowColSurface* geometryElement() const;
    Geometry::RowColSurface*	geometryElement();

    StepInterval<int>		rowRange() const;
    StepInterval<int>		colRange() const;
    StepInterval<int>		colRange(int row) const;

    virtual ObjectIterator*	createIterator(const TrcKeyZSampling* =0) const;

};

} // namespace EM
