/*
 * PIT.c
 *
 *  Created on: 20 Jun 2018
 *      Author: 11238639
 */

#include "types.h"
#include "MK70F12.h"
#include "PIT.h"

/* for sake of ease callback will be to run TakeSample in Sampler */
#include "Sampler.h"

// callbacks
static void (*PIT0Callback)(void*);
static void (*PIT1Callback)(void*);

static void * UserArguments;
static uint32_t ticksPerNanosecond;
static uint32_t nanoSecondsPerTick;
static uint32_t NANOSECOND = 1000000000;

/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments, const uint8_t pitIndex) {

  // PIT_LDVAL(pitIndex);

  // Enable clock gate to PIT module
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

  // Enable timer
  PIT_MCR &= ~PIT_MCR_MDIS_MASK;

  // set timer to freeze when debugging
  /* PIT_MCR |= PIT_MCR_FRZ_MASK; */

  // Set variable that can convert moduleClk ticks to nanoseconds
  ticksPerNanosecond = moduleClk / NANOSECOND;

  nanoSecondsPerTick = NANOSECOND / moduleClk;

  //store the userFunction
  /* Callback = userFunction; */

  switch(pitIndex)
  {
    case 0:
      PIT0Callback = userFunction;
      break;
    case 1:
      PIT1Callback = userFunction;
      break;
  }

  // store the user arguments
  UserArguments = userArguments;

  // Clear any pending interrupts on PIT(pitIndex)
  NVICICPR2 = (1 << (4 + pitIndex));
  // Enable interrupts from PIT(pitIndex) module
  NVICISER2 = (1 << (4 + pitIndex));

  return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart, const uint8_t pitIndex) {

  if (restart) {
    // Disable the timer and interrupts first
    PIT_Enable(false, pitIndex);

    // Clear pending interrupts on the PIT (w1c)
    PIT_TFLG(pitIndex) = PIT_TFLG_TIF_MASK;

  }

  // Setting PIT_LDVAL to period in nanoseconds converted to moduleClk ticks
  // PIT_LDVAL0 = period * ticksPerNanosecond;

  PIT_LDVAL(pitIndex) = (period / nanoSecondsPerTick) - 1;

  // Enable the timer and interrupts
  PIT_Enable(true, pitIndex);

}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable, const uint8_t pitIndex) {

  if (enable) {
    // Enable timer and interrupts
    PIT_TCTRL(pitIndex) |= (PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
  } else {
    // Disables timer and interrupts
    PIT_TCTRL(pitIndex) &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
  }

}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT0_ISR(void) {
  PIT_TFLG0 = PIT_TFLG_TIF_MASK;

  if (PIT0Callback) {
       (*PIT0Callback)(UserArguments);
   }

}

void __attribute__ ((interrupt)) PIT1_ISR(void) {
  PIT_TFLG1 = PIT_TFLG_TIF_MASK;

  if (PIT1Callback) {
       (*PIT1Callback)(UserArguments);
   }

}
