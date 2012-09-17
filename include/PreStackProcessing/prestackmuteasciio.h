#ifndef prestackmuteasciio_h
#define prestackmuteasciio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.h,v 1.7 2010/08/04 14:49:36 cvsbert Exp $
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
