#ifndef M68KCONF__HEADER
#define M68KCONF__HEADER

#define M68K_OPT_OFF			 0
#define M68K_OPT_ON				 1
#define M68K_OPT_SPECIFY_HANDLER 2

#define M68K_COMPILE_FOR_MAME M68K_OPT_OFF

/* Turn ON if you want to use the following M68K variants */
#define M68K_EMULATE_010   M68K_OPT_ON
#define M68K_EMULATE_EC020 M68K_OPT_OFF
#define M68K_EMULATE_020   M68K_OPT_OFF
#define M68K_EMULATE_030   M68K_OPT_OFF
#define M68K_EMULATE_040   M68K_OPT_OFF

#define M68K_SEPARATE_READS		   M68K_OPT_OFF
#define M68K_SIMULATE_PD_WRITES	   M68K_OPT_ON
#define M68K_EMULATE_TRACE		   M68K_OPT_OFF
#define M68K_EMULATE_PREFETCH	   M68K_OPT_OFF
#define M68K_EMULATE_ADDRESS_ERROR M68K_OPT_ON
#define M68K_USE_64_BIT			   M68K_OPT_ON

#define M68K_EMULATE_INT_ACK	 M68K_OPT_OFF
#define M68K_INT_ACK_CALLBACK(A) cpu_int_ack(A)

#define M68K_EMULATE_BKPT_ACK	 M68K_OPT_ON
#define M68K_BKPT_ACK_CALLBACK() cpu_bkpt_ack()

#define M68K_EMULATE_RESET	  M68K_OPT_ON
#define M68K_RESET_CALLBACK() cpu_reset()

/* If ON, CPU will call the set fc callback on every memory access to
 * differentiate between user/supervisor, program/data access like a real
 * 68000 would.  This should be enabled and the callback should be set if you
 * want to properly emulate the m68010 or higher. (moves uses function codes
 * to read/write data from different address spaces)
 */
#define M68K_EMULATE_FC			M68K_OPT_OFF
#define M68K_SET_FC_CALLBACK(A) cpu_set_fc(A)

/* If ON, CPU will call the instruction hook callback before every
 * instruction.
 */
#define M68K_INSTRUCTION_HOOK		  M68K_OPT_OFF
#define M68K_INSTRUCTION_CALLBACK(pc) your_instruction_hook_function(pc)

#endif /* M68KCONF__HEADER */
