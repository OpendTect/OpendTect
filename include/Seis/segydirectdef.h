#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.2 2008-09-18 14:55:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "iopar.h"
#include "bufstringset.h"
#include "position.h"
#include "samplingdata.h"
class SeisTrc;
class Executor;
class TaskRunner;
class PosGeomDetector;
class SEGYSeisTrcTranslator;

namespace SEGY {

class Scanner;


class DirectDef
{
public:

    			DirectDef(const Scanner&);
    			DirectDef(const char*); // read from file
			~DirectDef();

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;

    Seis::GeomType	geomType() const	{ return geom_; }   
    const IOPar&	pars() const		{ return pars_; }

protected:

    Seis::GeomType	geom_;
    const IOPar		pars_;

    int			curfidx_;
    SEGYSeisTrcTranslator* tr_;

};

}

#endif
