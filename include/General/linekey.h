#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"


/*!
\brief Key for a line in a line set.
*/

mExpClass(General) LineKey : public BufferString
{
public:

			LineKey( const char* lk=0 )
				: BufferString(lk)	{}
			LineKey(const char*,const char* attrnm);
			LineKey( const IOPar& iop, bool liin )
				{ usePar(iop,liin); }
    bool		operator ==(const LineKey&) const;
    bool		operator ==( const char* lk ) const
				{ return *this == LineKey(lk); }

    BufferString	lineName() const;
    BufferString	attrName() const;
    void		setLineName( const char* lnm )
				{ *this = LineKey( lnm, attrName() ); }
    void		setAttrName( const char* anm )
				{ *this = LineKey( lineName(), anm ); }

    void		fillPar(IOPar&,bool linename_is_iopar_name) const;
    bool		usePar(const IOPar&,bool linename_is_iopar_name);

    static const char*	sKeyDefAttrib()		{ return ""; }
};


/*!
\brief Class providing a current line key.
*/

mExpClass(General) GeomIDProvider
{
public:

    virtual		~GeomIDProvider()		{}
    virtual Pos::GeomID	geomID() const			= 0;

};


