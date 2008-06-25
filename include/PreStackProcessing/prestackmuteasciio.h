#ifndef prestackmuteascii0_h
#define prestackmuteasciio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.h,v 1.3 2008-06-25 06:38:19 cvsumesh Exp $
________________________________________________________________________

-*/



#include "tableascio.h"
#include "mathfunc.h"

namespace Table { class FormatDesc; }

namespace PreStack
{

class MuteDef;

class MuteAscIO : public Table::AscIO
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
   bool				getMuteDef(MuteDef&,bool extrapol, 
					PointBasedMathFunction::InterpolType);
   bool				getMuteDef(MuteDef&,const BinID&,bool extrapol,
	   				PointBasedMathFunction::InterpolType);

protected:
	
   static void 			createDescBody(Table::FormatDesc&,bool havepos);
   std::istream&		strm_;	
};

} // namespace PreStack

#endif
