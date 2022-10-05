#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				~MuteAscIO();

   static Table::FormatDesc*	getDesc();
   static void			updateDesc(Table::FormatDesc&,bool haveposinfo);

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

   static void			createDescBody(Table::FormatDesc&,bool havepos);
   od_istream&			strm_;
};

} // namespace PreStack
