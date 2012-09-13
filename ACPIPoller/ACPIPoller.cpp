/*
 * Copyright (c) 2012 RehabMan. All rights reserved.
 *
 * TODO: Include information about open source license here.
 *
 */

#include <IOKit/IOCommandGate.h>
#include "ACPIPoller.h"

#define super IOService
OSDefineMetaClassAndStructors(ACPIPoller, IOService)

/******************************************************************************
 * ACPIPoller::init
 ******************************************************************************/
bool ACPIPoller::init(OSDictionary *dict)
{
    IOLog("ACPIPoller::init: Initializing\n");
    
    bool result = super::init(dict);
    m_pDevice = NULL;
    m_pWorkLoop = NULL;
    m_pTimer = NULL;
    m_fInTimer = false;
    m_pMethods = NULL;
    return result;
}

/******************************************************************************
 * ACPIPoller::free
 ******************************************************************************/
void ACPIPoller::free(void)
{
    DEBUG_LOG("ACPIPoller::free: Freeing\n");
    super::free();
}

/******************************************************************************
 * ACPIPoller::probe
 ******************************************************************************/
IOService *ACPIPoller::probe(IOService *provider, SInt32 *score)
{
    DEBUG_LOG("ACPIPoller::probe: Probing\n");
    IOService *result = super::probe(provider, score);
    return result;
}

/******************************************************************************
 * ACPIPoller::start
 ******************************************************************************/
bool ACPIPoller::start(IOService *provider)
{
    DEBUG_LOG("ACPIPoller::start: called\n");
    
    m_pDevice = OSDynamicCast(IOACPIPlatformDevice, provider);

    if (NULL == m_pDevice || !super::start(provider))
        return false;

    m_pWorkLoop = getWorkLoop();
    if (NULL == m_pWorkLoop)
        return false;

    // "Methods" property in plist tells us what ACPI methods to call
    m_pMethods = OSDynamicCast(OSArray, getProperty("Methods"));
    if (NULL == m_pMethods)
        return false;
    DEBUG_LOG("ACPIPoller::start: found %d methods to call\n", m_pMethods->getCount());

    // need a timer to kick off every second
    m_pTimer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &ACPIPoller::OnTimerEvent));
    if (NULL == m_pTimer)
        return false;
	if (kIOReturnSuccess != m_pWorkLoop->addEventSource(m_pTimer))
        return false;
    
	IOLog("ACPIPoller: Version 2012.0912 starting\n");
    
    // call it once
    OnTimerEvent();
    
	this->registerService(0);
    return true;
}

/******************************************************************************
 * ACPIPoller::OnTimerEvent
 ******************************************************************************/
IOReturn ACPIPoller::OnTimerEvent()
{
    if (m_fInTimer)
        return kIOReturnSuccess;
    
    m_fInTimer = true;
    
    if (NULL != m_pMethods)
    {
        int count = m_pMethods->getCount();
        for (int i = 0; i < count; i++)
        {
            OSString* method = OSDynamicCast(OSString, m_pMethods->getObject(i));
            if (NULL == method)
                continue;
            DEBUG_LOG("ACPIPoller::OnTimerEvent: calling ACPI method: %s\n", method->getCStringNoCopy());
            m_pDevice->evaluateObject(method->getCStringNoCopy());
        }
    }

    if (NULL != m_pTimer)
    {
        m_pTimer->cancelTimeout();
        m_pTimer->setTimeoutMS(1000);
    }
    
    m_fInTimer = false;
   
    return kIOReturnSuccess;
}

/******************************************************************************
 * ACPIPoller::stop
 ******************************************************************************/
void ACPIPoller::stop(IOService *provider)
{
	DEBUG_LOG("ACPIPoller::stop: called\n");
    
    if (NULL != m_pTimer)
    {
        m_pTimer->cancelTimeout();
        m_pWorkLoop->removeEventSource(m_pTimer);
        m_pTimer->release();
        m_pTimer = NULL;
    }
	
    super::stop(provider);
}

