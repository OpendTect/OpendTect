#ifndef tabledef_h
#define tabledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2006
 RCS:		$Id: tabledef.h,v 1.1 2006-10-30 17:03:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "sets.h"
#include "rowcol.h"
#include "bufstringset.h"
#include <iostream>

namespace Table
{

/*!\brief Logical piece of information, present in tables.

 In many cases, data can be present or offered in various ways. For example, a
 position in the survey can be given as inline/crossline or X and Y. This would
 be specified as follows:

 FormatInfo fi( "Position" );
 fi.add( new BufferStringSet( {"Inline","Xline"}, 2 ) );
 fi.add( new BufferStringSet( {"X-coord","Y-coord"}, 2 ) );

*/

class FormatInfo : public NamedObject
{
public:
    			FormatInfo( const char* nm, bool optional=false,
				 const char* elemnm=0 )
			    : NamedObject(nm)
			    , optional_(optional)
    			{
			    if ( !elemnm || !*elemnm ) return;
			    BufferStringSet* s = new BufferStringSet;
			    s->add( elemnm );
			    add( s );
			}

    void		add( BufferStringSet* bss )	{ elements_ += bss; }


    /*!\brief Selected element/positioning */
    struct Selection
    {
			Selection() : elem_(0)	{}

	int		elem_;
	TypeSet<RowCol>	pos_;

    };

    ObjectSet<BufferStringSet>	elements_;
    bool		optional_;
    mutable Selection	selection_;

};


/*!\brief description of input our output table format */

class FormatDesc : public NamedObject
{
public:
    			FormatDesc( const char* nm )
			    : NamedObject(nm)
			    , nrhdrlines_(0)
			    , tokencol_(-1)		{}

    ObjectSet<FormatInfo> headerinfos_;
    ObjectSet<FormatInfo> bodyinfos_;

    int			nrhdrlines_;	//!< if < 0 token will be used
    BufferString	token_;
    int			tokencol_;	//!< if < 0 token can be in any col

};

}; // namespace Table


#endif
