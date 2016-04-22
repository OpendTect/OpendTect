#ifndef linekey_h
#define linekey_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2004
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

    mDeprecated		LineKey()
			    : BufferString("")	{}
    mDeprecated		LineKey(const char*,const char* attrnm);
    mDeprecated		LineKey( const IOPar& iop, bool liin )
				{ usePar(iop,liin); }
    bool		operator ==(const LineKey&) const;

    BufferString	lineName() const;
    BufferString	attrName() const;
    void		setLineName(const char* lnm);
    void		setAttrName(const char* anm);

    void		fillPar(IOPar&,bool linename_is_iopar_name) const;
    bool		usePar(const IOPar&,bool linename_is_iopar_name);

    static const char*	sKeyDefAttrib()		{ return ""; }
};


/*!
\brief Class providing a current GeomID.
*/

mExpClass(General) GeomIDProvider
{
public:

    virtual		~GeomIDProvider()		{}
    virtual Pos::GeomID	geomID() const			= 0;

};


#endif
