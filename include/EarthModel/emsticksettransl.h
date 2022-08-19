#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "transl.h"
#include "emstickset.h"

#include "rowcol.h"
#include "executor.h"

typedef EM::StickSet EMStickSet;

/*!
\brief EM::StickSet TranslatorGroup.
*/

mExpClass(EarthModel) EMStickSetTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(EMStickSet)
public:
				mDefEmptyTranslatorGroupConstructor(EMStickSet)

    const char*			defExtension() const override
    				{ return "stickset"; }

    static const char*		keyword;
};


/*!
\brief EM::StickSet Translator.
*/

mExpClass(EarthModel) EMStickSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(EMStickSet)

    virtual Executor*	reader( EM::StickSet&, Conn*,
				const char* formatfilename )	= 0;
			/*!< Executor is managed by client.
			     Conn becomes mine! */
    virtual Executor*	writer(const EM::StickSet&,Conn*,
				const char* formatfilename )	= 0;
			/*!< Executor is managed by client.
			     Conn becomes mine! */

    static Executor*	reader(EM::StickSet&,const IOObj*,BufferString& errmsg);
    static Executor*	writer(const EM::StickSet&,const IOObj*,
				BufferString& errmsg );

};


/*!
\brief Landmark EM::StickSet Translator.
*/

mExpClass(EarthModel) lmkEMStickSetTranslator : public EMStickSetTranslator
{				isTranslator(lmk,EMStickSet)
public:
			mDefEmptyTranslatorConstructor(lmk,EMStickSet)

    virtual Executor*	reader( EM::StickSet&, Conn*,
				const char* formatfilename );
    virtual Executor*	writer( const EM::StickSet&, Conn*,
				const char* formatfilename );

    virtual bool	isUserSelectable(bool) const	{ return false; }

    BufferString	warningmsg;

    static const char*	xstr;
    static const char*	ystr;
    static const char*	zstr;
    static const char*	pointtypestr;
    static const char*	domainstr;
    static const char*	surveystr;
    static const char*	domainunitstr;
    static const char*	distancunitestr;
    static const char*	lineidstr;
    static const char*	tracestr;
};


/*!
\brief Landmark EM::StickSet reader.
*/

mExpClass(EarthModel) lmkEMStickSetReader : public Executor
{
public:
			lmkEMStickSetReader(EM::StickSet&, Conn*,
						const char* formatfile );
			~lmkEMStickSetReader();
    virtual int         nextStep();

    virtual uiString	uiMessage() const;
    static const char*  streamerrmsg;

protected:

    EM::StickSet&	stickset;

    Conn*               conn;
    BufferString        msg;
    bool		useinlcrl;
    bool                error;

    EM::StickID		currentstick;
    EM::KnotID		currentknot;
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
\brief Landmark EM::StickSet writer.
*/

mExpClass(EarthModel) lmkEMStickSetWriter : public Executor
{
public:
			lmkEMStickSetWriter(const EM::StickSet&,
					    Conn*, const char* formatfile );
			~lmkEMStickSetWriter();
    virtual int         nextStep();

    virtual uiString	uiMessage() const { return "Writing knots"; }
    static const char*  streamerrmsg;

protected:
    void		fillBuffer( BufferString&, const Coord3&, int pt );

    const EM::StickSet&	stickset;

    Conn*               conn;
    BufferString        msg;

    int			currentsticknr;


    Interval<int>	pointtypeinterval;
    Interval<int>	xinterval;
    Interval<int>	yinterval;
    Interval<int>	zinterval;
    Interval<int>	domaininterval;
    Interval<int>	domainunitinterval;
    Interval<int>	distanceunitinterval;
};


#define mLMK_START_PT		1
#define mLMK_INTERMED_PT	2
#define mLMK_END_PT		3
#define mLMK_CONTROL_PT		4
