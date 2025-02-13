/**
 * @file EventCPP.h
 * @brief FreeRTOS Event Group Wrapper
 *
 * This file contains a set of lightweight wrappers for event groups using FreeRTOS
 *
 * @copyright (c) 2018-2024 Richard Damon
 * @author Richard Damon <richard.damon@gmail.com>
 * @parblock
 * MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * It is requested (but not required by license) that any bugs found or
 * improvements made be shared, preferably to the author.
 * @endparblock
 *
 * @ingroup FreeRTOSCpp
 */

#ifndef EVENTCPP_H
#define EVENTCPP_H

#include "FreeRTOScpp.h"
#include "event_groups.h"

/**
 * @def EVENT_BITS
 * Number of Bits usable as Event Bits
 * 
 * @ingroup FreeRTOSCpp 
 * 
 * @def EVENT_MASK
 * Mask of bits usable as Event Bits
 * @ingroup FreeRTOSCpp 
 */

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
  #define EVENT_BITS 8
  #define EVENT_MASK 0x00FFU
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
  #define EVENT_BITS 24
  #define EVENT_MASK 0x00FFFFFFUL
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
  #define EVENT_BITS 56
  #define EVENT_MASK 0x00FFFFFFFFFFFFFFULL
#endif /* if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS ) */

#if FREERTOSCPP_USE_NAMESPACE
namespace FreeRTOScpp {
#endif

class EventGroup {
public:
	EventGroup() {
#if( configSUPPORT_STATIC_ALLOCATION == 1 )
		eventHandle = xEventGroupCreateStatic(&eventBuffer);
#else
		eventHandle = xEventGroupCreate();
#endif
	}

	~EventGroup() {
#if( configSUPPORT_STATIC_ALLOCATION == 1 )
#else
		vEventGroupDelete(eventHandle);
#endif
	}

	/**
	 * Get Event Bits
	 *
	 */
	EventBits_t get() {
		return xEventGroupGetBits(eventHandle);
	}

	EventBits_t get_ISR() {
		return xEventGroupGetBitsFromISR(eventHandle);
	}

	/**
	 * Set Event Bits
	 *
	 * Set Event bits and activate all tasks waiting for those bits.
	 *
	 * @param bits The Event Bits to Set.
	 */
	EventBits_t set(EventBits_t bits) {
		return xEventGroupSetBits(eventHandle, bits);
	}

	EventBits_t set_ISR(EventBits_t bits, portBASE_TYPE& waswoken) {
		return xEventGroupSetBitsFromISR(eventHandle, bits, &waswoken);
	}

	/**
	 * Clear Event Bits
	 *
	 * @param bits The Event Bits to Set.
	 */
	EventBits_t clear(EventBits_t bits) {
		return xEventGroupClearBits(eventHandle, bits);
	}

	EventBits_t clear_ISR(EventBits_t bits) {
		return xEventGroupClearBitsFromISR(eventHandle, bits);
	}

	/**
	 * Event Group Sync
	 *
	 * Sets the set bits than wait for all of the wait bits, and then clear all those bits.
	 *
	 * @returns the value of the event group befor clearing the bits.
	 */
	EventBits_t sync(EventBits_t set, EventBits_t wait, TickType_t ticks = portMAX_DELAY){
		return xEventGroupSync(eventHandle, set, wait, ticks);
	}

#if FREERTOSCPP_USE_CHRONO
    /**
     * Event Group Sync
     *
     * Sets the set bits than wait for all of the wait bits, and then clear all those bits.
     *
     * @returns the value of the event group befor clearing the bits.
     */
    EventBits_t sync(EventBits_t set, EventBits_t wait, Time_ms ms){
        return xEventGroupSync(eventHandle, set, wait, ms2ticks(ms));
    }
#endif

	/**
	 * Wait for Event
	 *
	 * @param waitBits The bit(s) to wait for
	 * @param clear     If true, then the bits are cleared after the wait.
	 * @param all       If true, then wait for ALL the bits to be true, else for ANY of the bits
	 * @param ticks     How long to wait for the bits to be set
	 * @returns         The value of the event bits (before clearing) at the end of the wait.
	 */
	EventBits_t wait(EventBits_t waitBits, bool clear = true, bool all = false, TickType_t ticks = portMAX_DELAY) {
		return xEventGroupWaitBits(eventHandle, waitBits, clear, all, ticks);
	}
#if FREERTOSCPP_USE_CHRONO
    /**
     * Wait for Event
     *
     * @param waitBits The bit(s) to wait for
     * @param clear     If true, then the bits are cleared after the wait.
     * @param all       If true, then wait for ALL the bits to be true, else for ANY of the bits
     * @param ticks     How long to wait for the bits to be set
     * @returns         The value of the event bits (before clearing) at the end of the wait.
     */
    EventBits_t wait(EventBits_t waitBits, bool clear, bool all, Time_ms ms) {
        return xEventGroupWaitBits(eventHandle, waitBits, clear, all, ms2ticks(ms));
    }
#endif

protected:
	EventGroupHandle_t	eventHandle;
#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	StaticEventGroup_t	eventBuffer;
#endif

};

#if FREERTOSCPP_USE_NAMESPACE
}   // namespace FreeRTOScpp
#endif


#endif
