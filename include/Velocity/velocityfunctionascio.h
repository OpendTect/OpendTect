#ifndef velocitfunctionyasciio_h
#define velocitfunctionyasciio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: velocityfunctionascio.h,v 1.2 2009-04-29 16:25:05 cvskris Exp $
________________________________________________________________________

-*/

#include "tableascio.h"
#include "task.h"

namespace Table { class FormatDesc; }
class BinIDValueSet;
class TaskRunner;

namespace Vel
{

mClass FunctionAscIO : public Table::AscIO, public SequentialTask
{
public:
    				FunctionAscIO( const Table::FormatDesc& fd,
					       std::istream& stm,
				       	       int filesizeinkb=-1 );
   static Table::FormatDesc*	getDesc();
   static void			updateDesc(Table::FormatDesc&);

   float 			getUdfVal() const;
   bool				isXY() const;
   void				setOutput(BinIDValueSet& bvs)
       				{ output_ = &bvs; first_ = true; }

protected:
   int				nextStep();
   od_int64			nrDone() const { return nrdone_/1024; }
   const char*			nrDoneText() const { return "KBytes read"; }
   od_int64			totalNr() const { return nrbytes_; }
   static void			createDescBody(Table::FormatDesc&);

   std::istream&		strm_;   
   BinIDValueSet*		output_;
   bool				first_;
   int				nrdone_;
   int				nrbytes_;
};

} // namespace Vel


#endif
