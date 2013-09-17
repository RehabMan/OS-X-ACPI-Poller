/*
 * Copyright (c) 2012 RehabMan. All rights reserved.
 *
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along 
 *  with this program; if not, write to the Free Software Foundation, Inc., 
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <IOKit/IOCommandGate.h>
#include "ACPIPoller.h"

OSDefineMetaClassAndStructors(org_rehabman_ACPIPoller, IOService)

/******************************************************************************
 * ACPIPoller::init
 ******************************************************************************/
bool ACPIPoller::init(OSDictionary *dict)
{
    DEBUG_LOG("ACPIPoller::init: Initializing\n");
    
    bool result = super::init(dict);
    m_pDevice = NULL;
    m_pWorkLoop = NULL;
    m_pTimer = NULL;
    m_fInTimer = false;
    m_pMethods = NULL;
    return result;
}

/******************************************************************************
 * ACPIPoller::probe
 ******************************************************************************/
IOService *ACPIPoller::probe(IOService *provider, SInt32 *score)
{
    DEBUG_LOG("ACPIPoller::probe: Probing\n");
    
    IOService *result = super::probe(provider, score);
    IOACPIPlatformDevice* pDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    
    // "Methods" property in plist tells us what ACPI methods to call
    OSArray* pMethods = OSDynamicCast(OSArray, getProperty("Methods"));
    if (NULL == pMethods)
        return false;
    
    // check to be sure there will be work to do in timer, because otherwise
    // there is no reason to be here.
    int count = pMethods->getCount();
    int i = 0;
    for (/*nothing*/; i < count; i++)
    {
        OSString* method = OSDynamicCast(OSString, pMethods->getObject(i));
        if (NULL != method)
        {
            if (kIOReturnSuccess == pDevice->validateObject(method->getCStringNoCopy()))
                break;
            DEBUG_LOG("ACPIPoller::probe: unable to validate method '%s'\n", method->getCStringNoCopy());
        }
    }
    // if no methods validated, then fail the start (nothing to do)
    if (i >= count)
    {
        DEBUG_LOG("ACPIPoller::probe: unable to validate any methods\n");
        return NULL;
    }
    
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

    // "Methods" property in plist tells us what ACPI methods to call
    OSArray* pMethods = OSDynamicCast(OSArray, getProperty("Methods"));
    if (NULL == pMethods)
        return false;
    DEBUG_LOG("ACPIPoller::start: found %d methods to call\n", pMethods->getCount());
    
    // need a work loop to send timer events to
    m_pWorkLoop = getWorkLoop();
    if (NULL == m_pWorkLoop)
    {
        m_pMethods = NULL;
        return false;
    }
    m_pWorkLoop->retain();

    // need to hold a copy of the methods array for use in OnTimerEvent
    m_pMethods = pMethods;
    m_pMethods->retain();

    // need a timer to kick off every second
    m_pTimer = IOTimerEventSource::timerEventSource(this,
        OSMemberFunctionCast(IOTimerEventSource::Action, this, &ACPIPoller::OnTimerEvent));
    if (NULL == m_pTimer)
        return false;
	if (kIOReturnSuccess != m_pWorkLoop->addEventSource(m_pTimer))
        return false;
    
	IOLog("ACPIPoller: Version 0.8.0 starting\n");
    
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
    if (NULL != m_pWorkLoop)
    {
        m_pWorkLoop->release();
        m_pWorkLoop = NULL;
    }
    if (NULL != m_pMethods)
    {
        m_pMethods->release();
        m_pMethods = NULL;
    }
	
    super::stop(provider);
}

