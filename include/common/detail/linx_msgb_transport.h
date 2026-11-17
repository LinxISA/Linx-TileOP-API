// The public header supplies the native register operations. Tests supply an
// instrumented register bank to check the same protocol on the host.
#ifndef LINX_MSGB_TRANSPORT_H
#define LINX_MSGB_TRANSPORT_H

#include "linx_msgb_registers.h"

enum { LINX_MSGB_MAX_WORDS = 10 };

typedef enum {
    LINX_MSGB_NONE = 0,
    LINX_MSGB_OK = 1,
    LINX_MSGB_DUPLICATE = 2,
    LINX_MSGB_EMPTY = 3,
    LINX_MSGB_BUSY = 4,
    LINX_MSGB_NO_CLAIM = 5,
    LINX_MSGB_NOT_OWNER = 6,
    LINX_MSGB_TOKEN_MISMATCH = 7,
    LINX_MSGB_STALE_EPOCH = 8,
    LINX_MSGB_BAD_TOKEN = 9,
    LINX_MSGB_NOT_READ = 10,
    LINX_MSGB_EXHAUSTED = 11,
    LINX_MSGB_BAD_COMMAND = 12,
    LINX_MSGB_IO_ERROR = 13
} linx_msgb_result;

typedef struct {
    uint64_t hi;
    uint64_t lo;
} linx_msgb_id;

typedef struct {
    uint32_t endpoint;
    uint64_t token;
} linx_msgb_claim;

typedef struct {
    linx_msgb_id id;
    linx_msgb_claim claim;
    uint32_t destination;
    unsigned count;
    unsigned acked;
    uint64_t words[LINX_MSGB_MAX_WORDS];
} linx_msgb_message;

static inline linx_msgb_result linx_msgb_detail_result(uint64_t state)
{
    unsigned code = (unsigned)(state & 0xffu);
    return code > LINX_MSGB_NONE && code <= LINX_MSGB_IO_ERROR
        ? (linx_msgb_result)code : LINX_MSGB_IO_ERROR;
}

// On OK or DUPLICATE, id identifies the admitted message. A fresh call is a
// new dynamic send. Only architectural replay of an occurrence reuses its ID.
static inline linx_msgb_result linx_msgb_try_send(uint32_t destination,
        const uint64_t *words, size_t count, linx_msgb_id *id)
{
    if (count > LINX_MSGB_MAX_WORDS || (count && !words) || !id)
        return LINX_MSGB_BAD_COMMAND;

    // A data write checks the enabled bank. With no data, use CLAIM=0:
    // it has the same write preflight and leaves an existing RX claim alone.
    // Otherwise BSE on a disabled bank would issue a legacy scheduling event.
    if (count == 0)
        linx_msgb_io_write(LINX_MSGB_RXCLAIM, 0);
    for (unsigned i = 0; i < count; ++i)
        linx_msgb_io_stage(i, words[i]);
    linx_msgb_io_send(((uint64_t)count << 32) | destination);
    uint64_t status = linx_msgb_io_read(LINX_MSGB_TXSTAT);
    if (!(status & 0x100u))
        return LINX_MSGB_IO_ERROR;
    linx_msgb_result result = linx_msgb_detail_result(status);
    if (result == LINX_MSGB_OK || result == LINX_MSGB_DUPLICATE) {
        id->hi = linx_msgb_io_read(LINX_MSGB_TXID_HI);
        id->lo = linx_msgb_io_read(LINX_MSGB_TXID_LO);
    }
    return result;
}

static inline linx_msgb_result linx_msgb_detail_snapshot(uint64_t state,
                                                          linx_msgb_message *out)
{
    if (!(state & 0x100u))
        return LINX_MSGB_NO_CLAIM;
    unsigned count = (unsigned)((state >> 12) & 0xfu);
    uint64_t control = linx_msgb_io_read(LINX_MSGB_RXCONTROL);
    if (count > LINX_MSGB_MAX_WORDS || (control >> 32) != count)
        return LINX_MSGB_IO_ERROR;

    out->destination = (uint32_t)control;
    out->count = count;
    out->acked = (unsigned)((state >> 9) & 1u);
    out->claim.endpoint = (uint32_t)linx_msgb_io_read(LINX_MSGB_ENDPOINT);
    out->claim.token = linx_msgb_io_read(LINX_MSGB_RXTOKEN);
    out->id.hi = linx_msgb_io_read(LINX_MSGB_RXID_HI);
    out->id.lo = linx_msgb_io_read(LINX_MSGB_RXID_LO);
    for (unsigned i = 0; i < count; ++i)
        out->words[i] = linx_msgb_io_load(i);
    return LINX_MSGB_OK;
}

// CLAIM=1. BUSY may describe a bank which already owns a snapshot.
static inline linx_msgb_result linx_msgb_try_claim(linx_msgb_message *out)
{
    if (!out)
        return LINX_MSGB_BAD_COMMAND;
    linx_msgb_io_write(LINX_MSGB_RXCLAIM, 1);
    uint64_t state = linx_msgb_io_read(LINX_MSGB_RXSTATE);
    linx_msgb_result result = linx_msgb_detail_result(state);
    if (result != LINX_MSGB_OK)
        return result;
    return linx_msgb_detail_snapshot(state, out);
}

// Read a retained snapshot, including after ACK and before exact RELEASE.
static inline linx_msgb_result linx_msgb_read_claim(linx_msgb_message *out)
{
    if (!out)
        return LINX_MSGB_BAD_COMMAND;
    return linx_msgb_detail_snapshot(linx_msgb_io_read(LINX_MSGB_RXSTATE), out);
}

static inline linx_msgb_result linx_msgb_ack(linx_msgb_claim claim)
{
    if (linx_msgb_io_read(LINX_MSGB_ENDPOINT) != claim.endpoint)
        return LINX_MSGB_NOT_OWNER;
    linx_msgb_io_write(LINX_MSGB_ACK, claim.token);
    uint64_t status = linx_msgb_io_read(LINX_MSGB_ACKSTAT);
    if (!(status & 0x100u) || linx_msgb_io_read(LINX_MSGB_ACKID) != claim.token)
        return LINX_MSGB_IO_ERROR;
    return linx_msgb_detail_result(status);
}

static inline linx_msgb_result linx_msgb_release(linx_msgb_claim claim)
{
    if (linx_msgb_io_read(LINX_MSGB_ENDPOINT) != claim.endpoint)
        return LINX_MSGB_NOT_OWNER;
    linx_msgb_io_write(LINX_MSGB_RXRELEASE, claim.token);
    return linx_msgb_detail_result(linx_msgb_io_read(LINX_MSGB_RXSTATE));
}

#endif
