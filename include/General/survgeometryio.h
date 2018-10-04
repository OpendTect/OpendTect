#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
________________________________________________________________________

*/

#include "generalmod.h"
#include "survgeom.h"


#define mDefGeomIOFactoryFns(baseclss,subnm) \
    static baseclss*	createNew()	{ return new baseclss##subnm; }

namespace Survey
{

/*!\brief creates entries and writes geometries to the 2D geometry repository */

mExpClass(General) GeometryWriter2D : public GeometryWriter
{
public:
			GeometryWriter2D(){};
			mDefGeomIOFactoryFns(GeometryWriter,2D)
    static void		initClass();

    virtual bool	write(const Geometry&,uiString&,const char*) const;
    virtual IOObj*	createEntry(const char*) const;
    virtual GeometryID	createNewGeomID(const char*) const;

};


/*!\brief implements the GeometryWriter interface but does nothing as
	    there is only one 3D geometry (the survey's) */

mExpClass(General) GeometryWriter3D : public GeometryWriter
{
public:

			GeometryWriter3D()	{}
    static void		initClass();
			mDefGeomIOFactoryFns(GeometryWriter,3D)

    virtual bool	write( const Geometry&, uiString&, const char* ) const
						{ return false; }
    virtual IOObj*	createEntry( const char* ) const
						{ return 0; }
    virtual GeometryID	createNewGeomID( const char* ) const
						{ return mUdfGeomID; }


};


/*!\brief gets info from the 2D geometry repository */

mExpClass(General) GeometryReader2D : public GeometryReader
{
public:

			GeometryReader2D()	{}
			mDefGeomIOFactoryFns(GeometryReader,2D)
    static void		initClass();

    bool		read(ObjectSet<Geometry>&,TaskRunner*) const;
    bool		updateGeometries(ObjectSet<Geometry>&,
					 TaskRunner*) const;

};


/*!\brief implements the GeometryReader interface but does nothing as there is
	    only one 3D geometry (the survey's) */

mExpClass(General) GeometryReader3D : public GeometryReader
{
public:

			GeometryReader3D()	{}
			mDefGeomIOFactoryFns(GeometryReader,3D)
    static void		initClass();

    bool		read( ObjectSet<Geometry>&, TaskRunner* ) const
						{ return false; }
    bool		updateGeometries( ObjectSet<Geometry>&,
					  TaskRunner* ) const
						{ return false; }

};

} // namespace Survey
