#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual void		removeAll();

    bool			isAtEdge(const EM::PosID&) const override;
    bool			isLoaded() const override;
    Executor*			saver() override;
    virtual Executor*		saver(IOObj*)		{ return 0;}
    Executor*			loader() override;

    const char*			dbInfo() const		 { return dbinfo.buf();}
    void			setDBInfo(const char* s) { dbinfo = s; }

    bool			usePar(const IOPar&) override;
    void			fillPar(IOPar&) const override;

    EMObjectIterator*		createIterator(
				const TrcKeyZSampling* =nullptr) const override;

    bool			enableGeometryChecks(bool) override;
    bool			isGeometryChecksEnabled() const override;

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

    Geometry::Element*	geometryElementInternal() override;

    mDeprecated("Use geometryElementInternal()")
    Geometry::Element*	sectionGeometryInternal(const SectionID&) override;

    BufferString	dbinfo;
};


} // namespace EM
