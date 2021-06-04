#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/



#include "prestackprocessingmod.h"
#include "mathfunc.h"
#include "tableascio.h"

namespace Table { class FormatDesc; }

namespace PreStack
{

class MuteDef;

/*!
\brief Ascii I/O for PreStack mute.
*/

mExpClass(PreStackProcessing) MuteAscIO : public Table::AscIO
{
public:
				MuteAscIO(const Table::FormatDesc&,od_istream&);

   static Table::FormatDesc*	getDesc();
   static void 			updateDesc(Table::FormatDesc&,bool haveposinfo);

   float			getUdfVal() const;
   bool				isXY() const;
   bool				getMuteDef(MuteDef&,bool extrapol=true, 
					   PointBasedMathFunction::InterpolType=
						PointBasedMathFunction::Linear);
   bool				getMuteDef(MuteDef&,const BinID&,
	   				   bool extrapol=true,
	   				   PointBasedMathFunction::InterpolType=
						PointBasedMathFunction::Linear);

protected:
	
   static void 			createDescBody(Table::FormatDesc&,bool havepos);
   od_istream&			strm_;	
};

} // namespace PreStack

