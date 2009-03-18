#ifndef velocitfunctionyasciio_h
#define velocitfunctionyasciio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: velocityfunctionascio.h,v 1.1 2009-03-18 18:45:26 cvskris Exp $
________________________________________________________________________

-*/

#include "tableascio.h"

namespace Table { class FormatDesc; }
class BinIDValueSet;

namespace Vel
{

mClass FunctionAscIO : public Table::AscIO
{
public:
    				FunctionAscIO( const Table::FormatDesc& fd,
					       std::istream& stm )
				    : Table::AscIO(fd)
				    , strm_(stm)		{}
   static Table::FormatDesc*	getDesc();
   static void			updateDesc(Table::FormatDesc&);

   float 			getUdfVal() const;
   bool				isXY() const;
   bool				getVelocityData(BinIDValueSet&);

protected:
   static void			createDescBody(Table::FormatDesc&);
   std::istream&		strm_;   
};

} // namespace Vel


#endif
