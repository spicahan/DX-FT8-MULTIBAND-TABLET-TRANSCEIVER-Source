/* ====================================================
 *  Public API implementation
 * ==================================================== */

#include <stdbool.h>
#include "decode_ft8.h"
#include "qso_display.h"
#include "main.h"

void autoseq_init(const char *myCall, const char *myGrid);

void autoseq_start_cq(void);

/* === Called for selected decode (manual response) === */
void autoseq_on_touch(const Decode *msg);

/* === Called for **every** new decode (auto response) === */
/* Return whether TX is needed */
bool autoseq_on_decode(const Decode *msg);

/* === Provide the message we should transmit this slot (if any) === */
bool autoseq_get_next_tx(char out_text[MAX_MSG_LEN]);

/* === Populate the string for displaying the current QSO state  === */
void autoseq_get_qso_state(char out_text[MAX_LINE_LEN]);

/* === Slot timer / time‑out manager === */
void autoseq_tick(void);
