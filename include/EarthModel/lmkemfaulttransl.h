#ifndef lmkemfaulttransl_h
#define lmkemfaulttransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emsurfacetr.h"
#include "rowcol.h"
#include "executor.h"

namespace EM { class Fault3D; };

/*!
\brief Landmark EM::Fault3D EMSurfaceTranslator.
*/

mExpClass(EarthModel) lmkEMFault3DTranslator : public EMSurfaceTranslator
{			isTranslator(lmk,EMFault3D)
public:
    			lmkEMFault3DTranslator(const char* nm,
					     const char* unm)
			    : EMSurfaceTranslator(nm,unm)	{}
    virtual		~lmkEMFault3DTranslator();

    virtual Executor*	reader(EM::Fault3D&,Conn*,const char* formatfilename);
    virtual Executor*	writer(const EM::Fault3D&,Conn*,
			       const char* formatfilename);

    bool		isReadDefault() const		{ return false; }

    BufferString	warningmsg;

    static const char*	xstr();
    static const char*	ystr();
    static const char*	zstr();
    static const char*	pointtypestr();
    static const char*	domainstr();
    static const char*	surveystr();
    static const char*	domainunitstr();
    static const char*	distancunitestr();
    static const char*	lineidstr();
    static const char*	tracestr();

    virtual Executor*	reader( EM::Surface& s )
			{ return EMSurfaceTranslator::reader(s); }
};


/*!
\brief Landmark EM::Fault3D reader.
*/

mExpClass(EarthModel) lmkEMFault3DReader : public Executor
{
public:
			lmkEMFault3DReader(EM::Fault3D&,Conn*,
					   const char* formatfile);
			~lmkEMFault3DReader();
    virtual int         nextStep();

    virtual const char* message() const;
    static const char*  streamerrmsg;

protected:

    EM::Fault3D&	fault;

    Conn*               conn;
    BufferString        msg;
    bool		useinlcrl;
    bool                error;

    int			lastpt;
    RowCol		lastnode;

    Interval<int>	xinterval;
    Interval<int>	yinterval;
    Interval<int>	zinterval;
    Interval<int>	lineidinterval;
    Interval<int>	traceinterval;
    Interval<int>	pointtypeinterval;
    Interval<int>	domaininterval;
    Interval<int>	domainunitinterval;
    Interval<int>	distancuniteinterval;
};


/*!
\brief Landmark EM::Fault3D writer. 
*/

class lmkEMFault3DWriter : public Executor
{
public:
			lmkEMFault3DWriter(const EM::Fault3D&,
					   Conn*,const char* formatfile);
			~lmkEMFault3DWriter();

    virtual int         nextStep();
    virtual const char* message() const;
    static const char*  streamerrmsg;

protected:

    const EM::Fault3D&	fault;

    Conn*               conn;
    BufferString        msg;
    bool                error;

    RowCol		lastnode;

    Interval<int>	pointtypeinterval;
    Interval<int>	xinterval;
    Interval<int>	yinterval;
    Interval<int>	zinterval;
    Interval<int>	domaininterval;
    Interval<int>	domainunitinterval;
    Interval<int>	distancuniteinterval;
};


#define mLMK_START_PT		1
#define mLMK_INTERMED_PT	2
#define mLMK_END_PT		3
#define mLMK_CONTROL_PT		4

#endif

