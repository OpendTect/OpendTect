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
#include "trckeyzsampling.h"
#include "executor.h"
#include "multiid.h" 

class SeisTrcWriter;
class SeisTrc;


namespace Attrib
{
    
class DataCubes;

/*!
\brief Writes Attribute DataCubes.
*/

mExpClass(AttributeEngine) DataCubesWriter : public Executor
{ mODTextTranslationClass(DataCubesWriter);
public:
    			DataCubesWriter(const MultiID&,const Attrib::DataCubes&,
				       const TypeSet<int>& cubeindices);
			~DataCubesWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<float>&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;		
    uiString		uiMessage() const	{ 
						return tr("Writing out data!"); 
						}
    uiString		uiNrDoneText() const	{ 
						return tr("Traces written:"); 
						}
    int			nextStep();

private:

   int				nrdone_;    
   int				totalnr_;
   const Attrib::DataCubes&	cube_;
   TrcKeySamplingIterator	iterator_;
   MultiID			mid_;
   SeisTrcWriter*		writer_;
   SeisTrc*			trc_;
   TypeSet<int>			cubeindices_;

   TrcKeySampling		tks_;
   Interval<float>		zrg_;
};

}; //namespace


#endif

