#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / June 2016
________________________________________________________________________

-*/

#include "picklocation.h"
#include "tableascio.h"
namespace Pick { class Set; }


mExpClass(General) PickSetAscIO : public Table::AscIO
{
public:
				PickSetAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool iszreq);
    static void			updateDesc(Table::FormatDesc&,bool iszreq);
    static void                 createDescBody(Table::FormatDesc*,bool iszreq);

    bool			isXY() const;
    bool			get(od_istream&,Pick::Set&,bool iszreq,
				    float zval) const;
};
