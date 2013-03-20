#ifndef geometryio_h
#define geometryio_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
 RCS:		$Id$
________________________________________________________________________

*/

#include "generalmod.h"
#include "survgeom.h"

namespace Survey
{

/*!
\brief 2D GeometryWriter.
*/

mExpClass(General) GeometryWriter2D : public GeometryWriter
{
public:
				GeometryWriter2D(){};
    static GeometryWriter*	create2DWriter()
				{ return new GeometryWriter2D; }
    static void			initClass();
    bool			write(Geometry&) const;
    IOObj*			createEntry(const char* name) const;

};


/*!
\brief 3D GeometryWriter.
*/

mExpClass(General) GeometryWriter3D : public GeometryWriter
{
public:
				GeometryWriter3D(){};
    static GeometryWriter*	create3DWriter()
				{ return new GeometryWriter3D; }
    static void			initClass();


};


/*!
\brief 2D GeometryReader.
*/

mExpClass(General) GeometryReader2D : public GeometryReader
{
public:
				GeometryReader2D(){};
    static GeometryReader*	create2DReader()
				{ return new GeometryReader2D; }
    static void			initClass();
    bool			read(ObjectSet<Geometry>&,TaskRunner*) const;


};


/*!
\brief 3D GeometryReader.
*/

mExpClass(General) GeometryReader3D : public GeometryReader
{
public:
				GeometryReader3D(){};
    static GeometryReader*	create3DReader()
				{ return new GeometryReader3D; }
    static void			initClass();


};

}

#endif
