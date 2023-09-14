#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "tableascio.h"
#include "task.h"

namespace Table { class FormatDesc; }
class BinIDValueSet;

namespace Vel
{

mExpClass(Velocity) FunctionAscIO : public Table::AscIO, public SequentialTask
{ mODTextTranslationClass(FunctionAscIO);
public:
				FunctionAscIO(const Table::FormatDesc& fd,
					od_istream&,
					Pos::GeomID geomid=Pos::GeomID::udf(),
					od_int64 filesizeinkb=-1);
				~FunctionAscIO();

   static Table::FormatDesc*	getDesc(bool is2d);
   static void			updateDesc(Table::FormatDesc&,bool is2d);

   float			getUdfVal() const;
   bool				isXY() const;
   void				setOutput(BinIDValueSet&);

protected:

   int				nextStep() override;
   od_int64			nrDone() const override { return nrdone_/1024; }
   uiString			uiNrDoneText() const override
				{ return tr("KBytes read"); }

   od_int64			totalNr() const override { return nrkbytes_; }
   static void			createDescBody(Table::FormatDesc&,bool is2d);

   od_istream&			strm_;
   BinIDValueSet*		output_				= nullptr;
   bool				first_				= true;
   od_int64			nrdone_				= 0;
   Pos::GeomID			geomid_;
   od_int64			nrkbytes_;
};

} // namespace Vel
