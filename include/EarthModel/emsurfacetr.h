#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emsurfaceiodata.h"
#include "transl.h"
#include "uistring.h"

class IOObj;
class Task;

namespace EM
{
    class dgbSurfaceReader;
    class FaultStickSet;
    class Fault3D;
    class FaultSet3D;
    class Horizon;
    class Horizon2D;
    class Horizon3D;
    class Surface;
}


/*!
\brief TranslatorGroup for EM::Horizon3D. Reads/writes 3D EM::Horizon3D to
storage.
*/

mExpClass(EarthModel) EMHorizon3DTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMHorizon3D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon3D)

    const char*			defExtension() const override { return "hor"; }
};


/*!
\brief TranslatorGroup for EM::Horizon2D.
*/

mExpClass(EarthModel) EMHorizon2DTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMHorizon2D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon2D)

    const char*			defExtension() const override { return "2dh"; }
};


/*!
\brief TranslatorGroup for EM::Horizon.
*/

mExpClass(EarthModel) EMAnyHorizonTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMAnyHorizon)
public:
			    mDefEmptyTranslatorGroupConstructor(EMAnyHorizon)
};


/*!
\brief TranslatorGroup for EM::Fault3D.
*/

mExpClass(EarthModel) EMFault3DTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMFault3D)
public:
			mDefEmptyTranslatorGroupConstructor(EMFault3D)

    const char*		defExtension() const override { return "flt"; }
};


/*!
\brief TranslatorGroup for EM::FaultSet3D.
*/

mExpClass(EarthModel) EMFaultSet3DTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMFaultSet3D)
public:
			mDefEmptyTranslatorGroupConstructor(EMFaultSet3D)
};


/*!
\brief TranslatorGroup for EM::FaultStickSet.
*/

mExpClass(EarthModel) EMFaultStickSetTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(EMFaultStickSet)
public:
			mDefEmptyTranslatorGroupConstructor(EMFaultStickSet)

    const char*		defExtension() const override { return "fss"; }
};


/*!
\brief Translator for EM::Surface.
*/

mExpClass(EarthModel) EMSurfaceTranslator : public Translator
{
public:
				~EMSurfaceTranslator();

    bool			startRead(const IOObj&);
    bool			startWrite(const EM::Surface&);

    EM::SurfaceIODataSelection& selections()		{ return sels_; }

    virtual Task*		reader(EM::Surface&)	{ return nullptr; }
				/*!< Task is managed by client. */
    Task*			writer(const IOObj&,bool fullimplremove=true);
				/*!< Task is managed by client. */
    virtual Task*		getAuxdataReader(EM::Surface&,int)
				/*!<selidx==-1: read all*/
				{ return nullptr; }
    virtual Task*		getAuxdataWriter(const EM::Surface&,int,
						 bool dooverwrite=false)
				{ return nullptr; }

    uiString			errMsg() const		{ return errmsg_; }

    bool			implRemove(const IOObj*,bool) const override;
    bool			implRename(const IOObj*,
					   const char*) const override;
    bool			implSetReadOnly(const IOObj*,
						bool) const override;

    static bool			getBinarySetting();

    void			setCreatedFrom( const char* src )
							{ crfrom_ = src; }

protected:
				EMSurfaceTranslator(const char* nm,
						    const char* usernm);

    IOObj*			ioobj_				= nullptr;
    EM::Surface*		surface_			= nullptr;
    uiString			errmsg_;
    EM::SurfaceIOData		sd_;
    EM::SurfaceIODataSelection	sels_;
    BufferString		crfrom_;

    virtual bool		prepRead()		{ return true; }
    virtual bool		prepWrite()		{ return true; }
    virtual Task*		getWriter()		{ return nullptr; }

    void			init(const EM::Surface*,const IOObj*);
    void			setIOObj(const IOObj*);
};


/*!
\brief dgb EMSurfaceTranslator
*/

mExpClass(EarthModel) dgbEMSurfaceTranslator : public EMSurfaceTranslator
{
public:
				~dgbEMSurfaceTranslator();

    Task*			reader(EM::Surface&) override;

protected:
				dgbEMSurfaceTranslator(const char* nm,
						       const char* usernm);

    static BufferString		createHOVName(const char* base,int idx);
    bool			setSurfaceTransform(const IOPar&);
    void			getSels(StepInterval<int>&,StepInterval<int>&);

    bool			prepRead() override;
    Task*			getWriter() override;

    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }

    EM::dgbSurfaceReader*	reader_				= nullptr;
};


/*!
\brief dgbEMSurfaceTranslator for EM::Horizon3D.
*/

mExpClass(EarthModel) dgbEMHorizon3DTranslator : public dgbEMSurfaceTranslator
{
isTranslator(dgb,EMHorizon3D)
public:
				dgbEMHorizon3DTranslator(const char* nm,
						       const char* usernm);
				~dgbEMHorizon3DTranslator();

    Task*			getAuxdataReader(EM::Surface&,int) override;
    Task*			getAuxdataWriter(const EM::Surface&,int,
						    bool ovwrt=false) override;

protected:
    bool			readOnlyZ() const override	{ return true; }
    bool			writeOnlyZ() const override	{ return true; }
    bool			hasRangeSelection() const override
				    { return true; }
};


/*!
\brief dgbEMSurfaceTranslator for EM::Horizon2D.
*/

mExpClass(EarthModel) dgbEMHorizon2DTranslator : public dgbEMSurfaceTranslator
{
isTranslator(dgb,EMHorizon2D)
public:
				dgbEMHorizon2DTranslator(const char* nm,
						       const char* usernm);
    				~dgbEMHorizon2DTranslator();

protected:
    bool			readOnlyZ() const override	{return false;}
    bool			writeOnlyZ() const override	{return true;}
    bool			hasRangeSelection() const override
				    {return false;}
};


/*!
\brief dgbEMSurfaceTranslator for EM::Fault3D.
*/

mExpClass(EarthModel) dgbEMFault3DTranslator : public dgbEMSurfaceTranslator
{
isTranslator(dgb,EMFault3D)
public:
				dgbEMFault3DTranslator(const char* nm,
						       const char* usernm);
				~dgbEMFault3DTranslator();

protected:
    bool			readOnlyZ() const override	{ return false;}
    bool			writeOnlyZ() const override	{ return false;}
    bool			hasRangeSelection() const override
				{ return false;}
};


/*!
\brief dgbEMSurfaceTranslator for EM::FaultStickSet.
*/

mExpClass(EarthModel) dgbEMFaultStickSetTranslator :
				public dgbEMSurfaceTranslator
{
isTranslator(dgb,EMFaultStickSet)
public:
				dgbEMFaultStickSetTranslator(const char* nm,
							    const char* usernm);
				~dgbEMFaultStickSetTranslator();

protected:
    bool			readOnlyZ() const override	{ return false;}
    bool			writeOnlyZ() const override	{ return false;}
    bool			hasRangeSelection() const override
				{ return false;}
};


/*!
\brief Translator for EM::FaultSet3D.
*/

mExpClass(EarthModel) EMFaultSet3DTranslator : public Translator
{
public:
				~EMFaultSet3DTranslator();

    virtual Task*		writer(const EM::FaultSet3D&,const IOObj&) = 0;
    virtual Task*		reader(EM::FaultSet3D&,const IOObj&)	= 0;

protected:
				EMFaultSet3DTranslator(const char* nm,
						       const char* usernm);
};


mExpClass(EarthModel) dgbEMFaultSet3DTranslator : public EMFaultSet3DTranslator
{
isTranslator(dgb,EMFaultSet3D)
public:
				dgbEMFaultSet3DTranslator(const char* nm,
						       const char* usernm);
				~dgbEMFaultSet3DTranslator();

    Task*			reader(EM::FaultSet3D&,const IOObj&) override;
    Task*			writer(const EM::FaultSet3D&,
				       const IOObj&) override;
};


namespace EM
{
    void			addPluginTranslators();
} // namespace EM
