#ifndef prestackmuteascii0_h
#define prestackmuteasciio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.h,v 1.5 2009-01-06 06:05:40 cvsranojay Exp $
________________________________________________________________________

-*/



#include "tableascio.h"
#include "mathfunc.h"

namespace Table { class FormatDesc; }

namespace PreStack
{

class MuteDef;

mClass MuteAscIO : public Table::AscIO
{
public:
				MuteAscIO( const Table::FormatDesc& fd,
				       	   std::istream& stm )
				    : Table::AscIO(fd)
		 		    , strm_(stm)		{}
   static Table::FormatDesc*	getDesc();
   static void 			updateDesc(Table::FormatDesc&,bool haveposinfo);

   float			getUdfVal() const;
   bool				isXY() const;
   bool				getMuteDef(MuteDef&,bool extrapol=true, 
					   PointBasedMathFunction::InterpolType=
					   PointBasedMathFunction::Linear);
   bool				getMuteDef(MuteDef&,const BinID&,
	   				   bool extrapol=true,
	   				   PointBasedMathFunction::InterpolType=					   PointBasedMathFunction::Linear);

protected:
	
   static void 			createDescBody(Table::FormatDesc&,bool havepos);
   std::istream&		strm_;	
};

} // namespace PreStack

#endif
