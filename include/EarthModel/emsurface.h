#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emobject.h"

class IOObj;
namespace Pos { class Filter; }

namespace EM
{

class EMManager;
class SurfaceGeometry;

/*!
\brief Base class for surfaces like horizons and faults. A surface is made up
of one or more segments or patches, so they can overlap.
*/

mExpClass(EarthModel) Surface : public EMObject
{
public:
    int				nrSections() const;
    EM::SectionID		sectionID(int) const;
    BufferString		sectionName(const SectionID&) const;
    bool			canSetSectionName() const;
    bool			setSectionName(const SectionID&,const char*,
					       bool addtohistory);
    bool			removeSection(SectionID,bool hist);

    virtual void		removeAll();

    bool			isAtEdge(const EM::PosID&) const;
    bool			isLoaded() const;
    virtual Executor*		saver();
    virtual Executor*		saver(IOObj*)		{ return 0;}
    virtual Executor*		loader();

    const char*			dbInfo() const		 { return dbinfo.buf();}
    void			setDBInfo(const char* s) { dbinfo = s; }

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

    virtual EMObjectIterator*	createIterator(const SectionID&,
					       const TrcKeyZSampling* =0) const;

    bool			enableGeometryChecks(bool);
    bool			isGeometryChecksEnabled() const;

    virtual SurfaceGeometry&		geometry()			= 0;
    virtual const SurfaceGeometry&	geometry() const;

    static BufferString		getParFileName(const IOObj&);
    static BufferString		getSetupFileName(const IOObj&);
    static BufferString		getParentChildFileName(const IOObj&);

    virtual void		apply(const Pos::Filter&);

protected:

    friend class		SurfaceGeometry;
    friend class		EMObject;

				Surface(EMManager&);
				~Surface();
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&);

    BufferString		dbinfo;
};


} // namespace EM

