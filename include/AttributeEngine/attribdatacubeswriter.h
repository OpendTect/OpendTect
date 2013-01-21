#ifndef attribdatacubeswriter_h
#define attribdatacubeswriter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "cubesampling.h"
#include "executor.h"
#include "multiid.h" 

class BinID;
class IOPar;
class SeisTrcWriter;
class SeisTrc;

namespace Attrib
{
    
class DataCubes;

/*!
\brief Writes Attribute DataCubes.
*/

mExpClass(AttributeEngine) DataCubesWriter : public Executor
{
public:
    			DataCubesWriter(const MultiID&,const Attrib::DataCubes&,
				       const TypeSet<int>& cubeindices);
			~DataCubesWriter();

    void		setSelection(const HorSampling&,const Interval<int>&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;		
    const char*		message() const		{ return "Writing out data!"; }
    const char*		nrDoneText() const	{ return "Traces written:"; }
    int			nextStep();

private:

   int				nrdone_;    
   int				totalnr_;
   const Attrib::DataCubes&	cube_;
   HorSamplingIterator		iterator_;
   BinID			currentpos_;
   MultiID			mid_;
   SeisTrcWriter*		writer_;
   SeisTrc*			trc_;
   TypeSet<int>			cubeindices_;

   HorSampling			hrg_;
   Interval<int>		zrg_;
};

}; //namespace


#endif

