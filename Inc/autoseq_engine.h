/* ====================================================
 *  Public API implementation
 * ==================================================== */

#include <stdbool.h>
#include "decode_ft8.h"
#include "main.h"

void autoseq_init(const char *myCall, const char *myGrid);

void autoseq_start_cq(void);

void autoseq_start_reply(const Decode *dxFrame);

/* === Called for **every** new decode === */
/* Return whether TX is needed */
bool autoseq_on_decode(const Decode *msg);

/* === Provide the message we should transmit this slot (if any) === */
bool autoseq_get_next_tx(char out_text[40]);

/* === Slot timer / time‑out manager === */
void autoseq_tick(void);
