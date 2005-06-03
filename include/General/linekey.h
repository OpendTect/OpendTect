#ifndef linekey_h
#define linekey_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: linekey.h,v 1.2 2005-06-03 10:36:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
class IOPar;


/*!\brief Key for a line in a line set */

class LineKey : public BufferString
{
public:

				LineKey( const char* lk=0 )
				    : BufferString(lk)	{}
				LineKey(const char*,const char* attrnm);
				LineKey( const IOPar& iop, bool liin )
				    			{ usePar(iop,liin); }

    BufferString	lineName() const;
    BufferString	attrName() const;
    void		setLineName( const char* lnm )
				{ *this = LineKey( lnm, attrName() ); }
    void		setAttrName( const char* anm )
				{ *this = LineKey( lineName(), anm ); }

    void		fillPar(IOPar&,bool linename_is_iopar_name) const;
    bool		usePar(const IOPar&,bool linename_is_iopar_name);

    static const char*	sKeyDefAttrib;
    static BufferString	defKey2DispName(const char* defkey,
	    				const char* ioobjnm=0);
    			//!< 'defkey' is a LineKey where the line name
    			//!< may be an IOObj ID
};



/*!\brief class providing a current line key */

class LineKeyProvider
{
public:

    virtual		~LineKeyProvider()		{}
    virtual LineKey	lineKey() const			= 0;

};


#endif
