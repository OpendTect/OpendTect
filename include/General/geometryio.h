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

mClass(General) GeometryWriter2D : public GeometryWriter
{
public:
				GeometryWriter2D(){};
    static GeometryWriter*	create2DWriter()
				{ return new GeometryWriter2D; }
    static void			initClass();
    bool			write(Geometry*);
    int				createEntry(const char* name);

};


mClass(General) GeometryWriter3D : public GeometryWriter
{
public:
				GeometryWriter3D(){};
    static GeometryWriter*	create3DWriter()
				{ return new GeometryWriter3D; }
    static void			initClass();


};


mClass(General) GeometryReader2D : public GeometryReader
{
public:
				GeometryReader2D(){};
    static GeometryReader*	create2DReader()
				{ return new GeometryReader2D; }
    static void			initClass();
    bool			read(ObjectSet<Geometry>&);


};


mClass(General) GeometryReader3D : public GeometryReader
{
public:
				GeometryReader3D(){};
    static GeometryReader*	create3DReader()
				{ return new GeometryReader3D; }
    static void			initClass();


};

}

#endif
