#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "tableascio.h"
#include "task.h"

namespace Table { class FormatDesc; }
class BinnedValueSet;

namespace Vel
{

mExpClass(Velocity) FunctionAscIO : public Table::AscIO, public SequentialTask
{ mODTextTranslationClass(FunctionAscIO);
public:

    mUseType( Table,	FormatDesc );

			FunctionAscIO(const FormatDesc&,od_istream&,
				      od_int64 filesizeinkb=-1);
   static FormatDesc*	getDesc();
   static void		updateDesc(FormatDesc&);

   float		getUdfVal() const;
   bool			isXY() const;
   void			setOutput(BinnedValueSet& bvs)
			{ output_ = &bvs; first_ = true; }

protected:

   int			nextStep();
   od_int64		nrDone() const { return nrdone_/1024; }
   uiString		nrDoneText() const { return tr("KBytes read"); }
   od_int64		totalNr() const { return nrkbytes_; }
   static void		createDescBody(FormatDesc&);

   od_istream&		strm_;
   BinnedValueSet*	output_;
   bool			first_;
   od_int64		nrdone_;
   od_int64		nrkbytes_;
};

} // namespace Vel
