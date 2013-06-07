#ifndef datacoldef_h
#define datacoldef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "bufstring.h"
class UnitOfMeasure;


/*!\brief Column definition in tabular data sets

  The ref_ is intended for whatever references that are important in the
  software but which should probably not be displayed to the user.

  All but the 'user name' is optional.

*/

mClass DataColDef
{
public:
				DataColDef( const char* nm, const char* ref=0,
					    const UnitOfMeasure* un=0 )
				: name_(nm), ref_(ref), unit_(un)	{}
    bool			operator ==( const DataColDef& dcd ) const
				{ return name_ == dcd.name_
				      && ref_ == dcd.ref_
				      && unit_ == dcd.unit_; }

    BufferString		name_;
    BufferString		ref_;
    const UnitOfMeasure*	unit_;

    enum MatchLevel		{ Exact, Start, None };
    MatchLevel			compare(const DataColDef&,bool use_name) const;
				//!< if !use_name, matches ref_ .

    void			putTo(BufferString&) const;
    void			getFrom(const char*);

    static const DataColDef&	unknown();

};


#endif
