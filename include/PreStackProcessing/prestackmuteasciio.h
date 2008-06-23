#ifndef prestackmuteascii0_h
#define prestackmuteasciio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.h,v 1.1 2008-06-23 06:48:39 cvsumesh Exp $
________________________________________________________________________

-*/



#include "tableascio.h"
#include "mathfunc.h"
#include "survinfo.h"

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

   float			getUdfVal();
   bool				isXY() const;
   int 				getMuteDef(MuteDef&, bool, 
	                        PointBasedMathFunction::InterpolType );

protected:
	
   std::istream&		strm_;	
};

}; //namespace

#endif
