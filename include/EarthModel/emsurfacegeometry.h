#ifndef emsurfacegeometry_h
#define emsurfacegeometry_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "bufstringset.h"
#include "emposid.h"
#include "emobject.h"
#include "position.h"
#include "mathfunc.h"
#include "rowcolsurface.h"

template <class T, class AT> class TopList;


class RowCol;
template <class T> class Interval;
template <class T> class StepInterval;


namespace Geometry { class RowColSurface; };


namespace EM
{
class Surface;
class SurfaceIODataSelection;


mClass(EarthModel) SurfaceGeometry : public CallBacker
{
public:
    			SurfaceGeometry(Surface&);
    virtual		~SurfaceGeometry();
    virtual void	removeAll();

    virtual bool	enableChecks(bool yn);
    virtual bool	isChecksEnabled() const;
    virtual bool	isNodeOK(const PosID&) const;

    int			nrSections() const;
    SectionID		sectionID(int idx) const;
    SectionID		sectionID(const char*) const;
    bool		hasSection(const SectionID&) const;
    int			sectionIndex(const SectionID&) const;
    const char*		sectionName(const SectionID&) const;
    bool		setSectionName( const SectionID&, const char*,
	    				bool addtohistory );
    SectionID		addSection(const char* nm,bool addtohistory);
    SectionID		addSection(const char* nm,const SectionID&,
	    			   bool addtohistory);
    			/*!<Return false if the sectionid allready exists */
    virtual bool	removeSection(const SectionID&,bool addtohistory);
    virtual SectionID	cloneSection(const SectionID&);
    
    virtual const Geometry::Element*	sectionGeometry(const SectionID&) const;
    virtual Geometry::Element*		sectionGeometry(const SectionID&);
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
			      const MultiID* key=0);

    virtual int		findPos(const SectionID&,const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    virtual int		findPos(const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,TypeSet<PosID>*) const;
    virtual int		findPos(const CubeSampling&,TypeSet<PosID>*) const;

    virtual EMObjectIterator*	createIterator(const EM::SectionID&,
	    				       const CubeSampling* =0) const;


    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

    SectionID			addSectionInternal(Geometry::Element*,
					   const char* nm,const SectionID&,
					   bool addtohistory);


    virtual Geometry::Element*		createSectionGeometry() const = 0;
    void				geomChangeCB(CallBacker*);

    Surface&				surface_;
    ObjectSet<Geometry::Element>	sections_;
    TypeSet<SectionID>			sids_;
    BufferStringSet			sectionnames_;

    bool				changed_;

};


mClass(EarthModel) RowColSurfaceGeometry : public SurfaceGeometry
{
public:
    			RowColSurfaceGeometry(Surface&);
    virtual		~RowColSurfaceGeometry();


    const Geometry::RowColSurface* sectionGeometry(const SectionID&) const;
    Geometry::RowColSurface*	sectionGeometry(const SectionID&);

    StepInterval<int>		rowRange(const SectionID&) const;
    StepInterval<int>		rowRange() const;
    StepInterval<int>		colRange(const SectionID&,int row) const;
    StepInterval<int>		colRange() const;
    StepInterval<int>		colRange(int row) const;

    virtual EMObjectIterator*	createIterator(const EM::SectionID&,
	    				       const CubeSampling* =0) const;

};


}; // namespace EM

#endif

