/*
 * autoseq_engine.c – FT8 CQ/QSO auto‑sequencing engine (revised)
 *
 * This version consumes the **Decode** structure defined in decode_ft8.h, the
 * native output of *ft8_decode()* in the DX‑FT8 transceiver firmware, so no
 * wrapper conversion is required.  All logic remains the same as the QEX/WSJT‑X
 * state diagram (Tx1…Tx6) presented earlier.
 *
 * Public API (unchanged except for Decode use)
 * ────────────────────────────────────────────
 *   void  autoseq_init(const char *myCall, const char *myGrid);
 *   void  autoseq_start_cq(void);
 *   void  autoseq_start_reply(const Decode *dxFrame);
 *   void  autoseq_on_decode(const Decode *msg);   // <‑‑ uses Decode now
 *   bool  autoseq_get_next_tx(char out_text[40]); // returns plain text for gen_ft8()
 *   void  autoseq_tick(void);
 *
 * The transmit side now emits a ready‑formatted 3‑field FT8 text message (max
 * 22 chars, but we keep a 40‑char buffer for safety).  Hand this string to the
 * existing *queue_message_custom(text)* helper or directly into *gen_ft8()*.
 *
 * Copyright (c) 2025 <your‑callsign>.  MIT licence.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "autoseq_engine.h"
#include "gen_ft8.h" // For saving Target_*
#include "ADIF.h"    // For write_ADIF_Log()

/***** Compile‑time knobs *****/
#define MAX_TX_RETRY 5

/***** Identifiers for the six canonical FT8 messages *****/
typedef enum {
    TX_UNDEF = 0,
    TX1,   /*  <DXCALL> <MYCALL> <GRID>            */
    TX2,   /*  <DXCALL> <MYCALL> ##                */
    TX3,   /*  <DXCALL> <MYCALL> R##               */
    TX4,   /*  <DXCALL> <MYCALL> RR73 (or RRR)     */
    TX5,   /*  <DXCALL> <MYCALL> 73                */
    TX6    /*  CQ <MYCALL> <GRID>                  */
} tx_msg_t;

/***** High‑level auto‑sequencer states *****/
typedef enum {
    AS_IDLE = 0,
    AS_REPLYING,
    AS_REPORT,
    AS_ROGER_REPORT,
    AS_ROGERS,
    AS_SIGNOFF,
    AS_CALLING,
} autoseq_state_t;

/***** Control‑block *****/
typedef struct {
    autoseq_state_t state;
    tx_msg_t        next_tx;

    char mycall[14];
    char mygrid[7];
    char dxcall[14];
    char dxgrid[7];

    int  snr_tx;              /* SNR we report to DX (‑dB) */
    int  retry_counter;
    int  retry_limit;
    // TODO eliminate
    bool we_are_caller;       /* true => we started with CQ */
    bool logged;              /* true => QSO logged */

} autoseq_ctx_t;

static autoseq_ctx_t ctx;

/*************** Forward declarations ****************/ 
static void set_state(autoseq_state_t s, tx_msg_t first_tx, int limit);
static void format_tx_text(tx_msg_t id, char out[40]);
/******************************************************/

/* ====================================================
 *  Public API implementation
 * ==================================================== */

void autoseq_init(const char *myCall, const char *myGrid)
{
    memset(&ctx, 0, sizeof(ctx));
    strncpy(ctx.mycall, myCall, sizeof(ctx.mycall) - 1);
    strncpy(ctx.mygrid, myGrid, sizeof(ctx.mygrid) - 1);
    ctx.state = AS_IDLE;
}

void autoseq_start_cq(void)
{
    ctx.we_are_caller = true;
    set_state(AS_CALLING, TX6, 0); /* infinite CQ loop */
}

void autoseq_start_reply(const Decode *dxFrame)
{
    /* dxFrame comes from a double‑click: <DXCALL> CQ etc. */
    strncpy(ctx.dxcall, dxFrame->call_from, sizeof(ctx.dxcall) - 1);
    strncpy(ctx.dxgrid, dxFrame->locator,   sizeof(ctx.dxgrid) - 1);
    ctx.we_are_caller = false;
    set_state(AS_REPLYING, TX1, MAX_TX_RETRY);
}

/* === Called for **every** new decode === */
/* Return whether TX is needed */
bool autoseq_on_decode(const Decode *msg)
{
    if (!msg) return false;
    // Not addresses me, return false
    if (strncmp(msg->call_to, ctx.mycall, 14) != 0) {
        return false;
    }

    // If the current QSO is still in progress, we ignore different dxcall
    if (ctx.state == AS_REPLYING || ctx.state == AS_REPORT || ctx.state == AS_ROGER_REPORT) {
        if (strncmp(msg->call_from, ctx.dxcall, sizeof(ctx.dxcall)) != 0) {
            return false;
        }
    }
    strncpy(ctx.dxcall, msg->call_from, sizeof(ctx.dxcall));
    ctx.snr_tx = msg->snr;

    // TX type of received message
    tx_msg_t rcvd_msg_type = TX_UNDEF;
    if (msg->sequence == Seq_Locator) {
        rcvd_msg_type = TX1;
        strncpy(ctx.dxgrid, msg->locator, sizeof(ctx.dxgrid));
    } else {
        if (strncmp(msg->locator, "73", 2) == 0) {
            rcvd_msg_type = TX5;
        } 
        else if (strncmp(msg->locator, "RR73", 4) == 0)
        {
            rcvd_msg_type = TX4;
        }
        else if (strncmp(msg->locator, "RRR", 3) == 0)
        {
            rcvd_msg_type = TX4;
        }
        else if (msg->locator[0] == 'R')
        {
            rcvd_msg_type = TX3;
        }
        else {
            rcvd_msg_type = TX2;
        }
    }

    char rcvd_msg_type_str[15]; // "ST: , Rcvd TXx\n"
    snprintf(rcvd_msg_type_str, sizeof(rcvd_msg_type_str), "ST:%u, Rcvd TX%u", ctx.state, rcvd_msg_type);
    // _debug(rcvd_msg_type_str);

    // Populating Target_Call
    strncpy(Target_Call, msg->call_from, sizeof(Target_Call));

    // Populating Station_RSL
    if (rcvd_msg_type == TX2 || rcvd_msg_type == TX3) {
        Station_RSL = msg->received_snr;
    }

    switch (ctx.state) {
    /* ------------------------------------------------ CALLING (we sent CQ) */
    case AS_CALLING:
        switch (rcvd_msg_type) {
            case TX1:
                // Populate Target_Locator
                strncpy(Target_Locator, msg->locator, sizeof(Target_Locator));
                set_state(AS_REPORT, TX2, MAX_TX_RETRY);
                return true;
            case TX2:
                set_state(AS_ROGER_REPORT, TX3, MAX_TX_RETRY);
                return true;
            case TX3:
                set_state(AS_ROGERS, TX4, MAX_TX_RETRY);
                return true;
            default:
                return false;
        }

    /* ------------------------------------------------ REPLYING (we sent Tx1) */
    case AS_REPLYING:
        switch (rcvd_msg_type) {
            // Since we sent TX1, it doesn't make sense to respond to TX1
            // case TX1:
            //     set_state(AS_REPORT, TX2, MAX_TX_RETRY);
            //     return true;
            case TX2:
                set_state(AS_ROGER_REPORT, TX3, MAX_TX_RETRY);
                return true;
            case TX3:
                set_state(AS_ROGERS, TX4, MAX_TX_RETRY);
                return true;

            // QSO complete without signal report exchange
            case TX4:
            case TX5:
                set_state(AS_SIGNOFF, TX5, 0);
                return true;
            default:
                return false;
        }

    /* ------------------------------------------------ REPORT sent, waiting Roger */
    case AS_REPORT:
        switch (rcvd_msg_type) {
            // DX didn't copy our TX2 response, stay in the same state by returning false
            // case TX1:
            //     set_state(AS_REPORT, TX2, MAX_TX_RETRY);
            //     return true;

            // Since we sent TX2, it doesn't make sense to respond to TX2
            // case TX2:
            //     set_state(AS_ROGER_REPORT, TX3, MAX_TX_RETRY);
            //     return true;

            case TX3:
                set_state(AS_ROGERS, TX4, MAX_TX_RETRY);
                return true;
            // QSO complete without signal report exchange
            case TX4:
            case TX5:
                set_state(AS_SIGNOFF, TX5, 0);
                return true;
            default:
                return false;
        }

    /* ------------------------------------------------ Roger‑Report sent */
    case AS_ROGER_REPORT:
        switch (rcvd_msg_type) {
            // Since we sent TX1, it doesn't make sense to respond to TX1
            // case TX1:
            //     set_state(AS_REPORT, TX2, MAX_TX_RETRY);
            //     return true;

            // DX didn't copy our TX3 response, stay in the same state by returning false
            // case TX2:
            //     set_state(AS_ROGER_REPORT, TX3, MAX_TX_RETRY);
            //     return true;

            // Since we sent TX3, it doesn't make sense to respond to TX3
            // case TX3:
            //     set_state(AS_SIGNOFF, TX4, MAX_TX_RETRY);
            //     return true;

            // QSO complete
            case TX4:
            case TX5: // Be polite, echo back 73
                set_state(AS_SIGNOFF, TX5, 0);
                return true;
            default:
                return false;
        }
    
    case AS_ROGERS:
        switch (rcvd_msg_type) {
            // QSO complete
            case TX4:
            case TX5:
                set_state(AS_IDLE, TX_UNDEF, 0);
                break;
            default:
                break;
        }
        return false;

    case AS_SIGNOFF:
        switch (rcvd_msg_type) {
            // DX hasn't received our TX5. Retry
            case TX4:
                break;
            default:
                set_state(AS_IDLE, TX_UNDEF, 0);
                break;
        }
        return false;

    default:
        break;
    }
    return false;
}

/* === Provide the message we should transmit this slot (if any) === */
bool autoseq_get_next_tx(char out_text[40])
{
    if (ctx.next_tx == TX_UNDEF)
        return false;

    format_tx_text(ctx.next_tx, out_text);

    /* Bump retry counter */
    if (ctx.retry_limit) {
        if (ctx.retry_counter < ctx.retry_limit) {
            return true;
        }
        // Is this right?
        ctx.state = AS_SIGNOFF; /* give up */
    }

    // Is this right?
    ctx.next_tx = TX_UNDEF;
    ctx.logged = false;
    return true;
}

/* === Slot timer / time‑out manager === */
void autoseq_tick(void)
{
    if (ctx.next_tx == TX_UNDEF || ctx.state == AS_IDLE)
        return;

    switch (ctx.state) {
    case AS_REPLYING:
        if (ctx.retry_counter < ctx.retry_limit) {
            ctx.next_tx = TX1;
            ctx.retry_counter++;
        } else {
            ctx.state = AS_SIGNOFF;
            ctx.next_tx = TX5;
        }
        break;
    case AS_REPORT:
        if (ctx.retry_counter < ctx.retry_limit) {
            ctx.next_tx = TX2;
            ctx.retry_counter++;
        } else {
            ctx.state = AS_SIGNOFF;
            ctx.next_tx = TX5;
        }
        break;
    case AS_ROGER_REPORT:
        if (ctx.retry_counter < ctx.retry_limit) {
            ctx.next_tx = TX3;
            ctx.retry_counter++;
        } else {
            ctx.state = AS_SIGNOFF;
            ctx.next_tx = TX5;
        }
        break;
    case AS_ROGERS:
        if (ctx.retry_counter < ctx.retry_limit) {
            ctx.next_tx = TX4;
            ctx.retry_counter++;
        } else {
            ctx.state = AS_IDLE;
            ctx.next_tx = TX_UNDEF;
            ctx.logged = false;
        }
        break;
    case AS_SIGNOFF:
        if (ctx.retry_counter < ctx.retry_limit) {
            ctx.next_tx = TX5;
            ctx.retry_counter++;
        } else {
            ctx.state = AS_IDLE;
            ctx.next_tx = TX_UNDEF;
            ctx.logged = false;
        }
        break;
    default:
        break;
    }
}

/* ================================================================
 *                Internal helpers
 * ================================================================ */

static void set_state(autoseq_state_t s, tx_msg_t first_tx, int limit)
{
    ctx.state        = s;
    ctx.next_tx      = first_tx;
    ctx.retry_counter = 0;
    ctx.retry_limit   = limit;
}

/* Build printable FT8 text ("<CALL> <CALL> <LOC/RPT>") */
static void format_tx_text(tx_msg_t id, char out[40])
{
    memset(out, 0, 40);

    switch (id) {
    case TX1:
        snprintf(out, 40, "%s %s %s", ctx.dxcall, ctx.mycall, ctx.mygrid);
        break;
    case TX2:
        snprintf(out, 40, "%s %s %+d", ctx.dxcall, ctx.mycall, ctx.snr_tx);
        Target_RSL = ctx.snr_tx;
        break;
    case TX3:
        snprintf(out, 40, "%s %s R%+d", ctx.dxcall, ctx.mycall, ctx.snr_tx);
        Target_RSL = ctx.snr_tx;
        break;
    case TX4:
        snprintf(out, 40, "%s %s RR73", ctx.dxcall, ctx.mycall);
        if (!ctx.logged) {
            write_ADIF_Log();
            ctx.logged = true;
        }
        break;
    case TX5:
        snprintf(out, 40, "%s %s 73", ctx.dxcall, ctx.mycall);
        if (!ctx.logged) {
            write_ADIF_Log();
            ctx.logged = true;
        }
        break;
    case TX6:
        snprintf(out, 40, "CQ %s %s", ctx.mycall, ctx.mygrid);
        break;
    default:
        break;
    }
}
