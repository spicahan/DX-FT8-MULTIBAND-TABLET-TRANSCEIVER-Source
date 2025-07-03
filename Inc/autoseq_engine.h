/* ====================================================
 *  Public API implementation
 * ==================================================== */

#include <stdbool.h>
#include "decode_ft8.h"
#include "qso_display.h"
#include "main.h"

extern void autoseq_init(const char *myCall, const char *myGrid);

void autoseq_start_cq(void);

/* === Called for selected decode (manual response) === */
void autoseq_on_touch(const Decode *msg);

/* === Called for the entire decode array === */
void autoseq_on_decodes(const Decode *messages, int num_decoded);

/* === Provide the message we should transmit === */
bool autoseq_get_next_tx(char out_text[MAX_MSG_LEN]);

/* === Populate the string for displaying the current QSO state  === */
void autoseq_get_qso_state(char lines[][MAX_LINE_LEN]);

/* === Slot timer / time‑out manager === */
void autoseq_tick(void);
