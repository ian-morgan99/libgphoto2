#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gphoto2/gphoto2-result.h>

#include "pentax-utils.h"

/* Deterministic tests for pentax_reconcile_extra_candidates() (issue #73):
 * the bounded loop that consumes extra transfer candidates left behind by a
 * dual-format exposure.  All camera behaviour is mocked through the
 * PentaxReconcileOps callbacks, so no wire IO happens. */

#define CHECK(expression) do { \
        if (!(expression)) { \
                fprintf (stderr, "FAIL line %d: %s\n", __LINE__, #expression); \
                return 1; \
        } \
} while (0)

static void
put_u32le (unsigned char *data, size_t offset, uint32_t value)
{
        data[offset] = (unsigned char)value;
        data[offset + 1] = (unsigned char)(value >> 8);
        data[offset + 2] = (unsigned char)(value >> 16);
        data[offset + 3] = (unsigned char)(value >> 24);
}

/* Build a candidate-info payload that pentax_candidate_filename() accepts:
 * bytes 0-2 reserved, byte 3 = UTF-16LE character count, then the name. */
static size_t
make_candidate_info (const char *name, unsigned char *out, size_t out_size)
{
        size_t len = strlen (name), i;

        if (4 + len * 2 > out_size)
                return 0;
        memset (out, 0, 4);
        out[3] = (unsigned char)len;
        for (i = 0; i < len; i++) {
                out[4 + i * 2] = (unsigned char)name[i];
                out[5 + i * 2] = 0;
        }
        return 4 + len * 2;
}

typedef struct {
        /* Candidate handles still pending, consumed one per iteration. */
        uint32_t handles[8];
        int handle_count;
        const char *names[8];
        /* Fault injection. */
        int conditions_fail_first;   /* fail the first N get_conditions calls */
        int conditions_empty_first;  /* report empty for the first N reads */
        int writing_format;          /* +524: 2 means RAW+JPEG */
        int conditions_calls;
        int transfer_error;          /* non-zero: transfer_candidate fails */
        int delete_error;            /* non-zero: delete_candidate fails */
        int cancel_after;            /* cancel once this many candidates done */
        int info_fail;              /* non-zero: get_candidate_info fails */
        /* Publication simulation (o-v12m companion UAF regression): when
         * set, transfer_candidate simulates gp_file_set_data_and_size()
         * taking ownership of the buffer and clears it, so the reconcile
         * loop must not free the published bytes again. */
        int publish;
        unsigned char *published[4];
        int published_count;
        /* Observations. */
        int transfer_calls;
        int delete_calls;
} MockReconcile;

static int
mock_get_conditions (void *user_data, unsigned char **data, size_t *size)
{
        MockReconcile *mock = user_data;
        unsigned char *blob;

        mock->conditions_calls++;
        if (mock->conditions_fail_first > 0 &&
            mock->conditions_calls <= mock->conditions_fail_first) {
                *data = NULL;
                *size = 0;
                return GP_ERROR_IO;
        }
        size_t blob_size = mock->writing_format ? 576 : PENTAX_CONDITIONS_MIN_SIZE;
        blob = calloc (1, blob_size);
        if (!blob) {
                *data = NULL;
                *size = 0;
                return GP_ERROR_NO_MEMORY;
        }
        if (mock->handle_count > 0 &&
            mock->conditions_calls > mock->conditions_empty_first) {
                put_u32le (blob, 32, 1);
                put_u32le (blob, 36, mock->handles[0]);
        }
        if (mock->writing_format)
                put_u32le (blob, 524, (uint32_t)mock->writing_format);
        *data = blob;
        *size = blob_size;
        return GP_OK;
}

static int
mock_get_candidate_info (void *user_data, unsigned char **data, size_t *size)
{
        MockReconcile *mock = user_data;
        unsigned char *info;
        size_t len;

        if (mock->info_fail) {
                *data = NULL;
                *size = 0;
                return GP_ERROR_IO;
        }
        if (mock->handle_count <= 0 || !mock->names[0]) {
                *data = NULL;
                *size = 0;
                return GP_OK;
        }
        info = calloc (1, 256);
        if (!info) {
                *data = NULL;
                *size = 0;
                return GP_ERROR_NO_MEMORY;
        }
        len = make_candidate_info (mock->names[0], info, 256);
        *data = info;
        *size = len;
        return GP_OK;
}

static int
mock_transfer_candidate (void *user_data, PentaxCaptureBuffer *buffer)
{
        MockReconcile *mock = user_data;

        mock->transfer_calls++;
        if (mock->transfer_error)
                return mock->transfer_error;
        buffer->data = malloc (6);
        if (!buffer->data)
                return GP_ERROR_NO_MEMORY;
        memcpy (buffer->data, "EXTRA", 6);
        buffer->size = 5;
        if (mock->publish) {
                /* Simulate gp_file_set_data_and_size(): ownership of the
                 * buffer transfers to the published CameraFile, so the
                 * transfer object is cleared and the reconcile loop must
                 * not free these bytes again (o-v12m companion UAF). */
                if (mock->published_count < 4)
                        mock->published[mock->published_count++] = buffer->data;
                buffer->data = NULL;
                buffer->size = 0;
        }
        return GP_OK;
}

static int
mock_delete_candidate (void *user_data)
{
        MockReconcile *mock = user_data;

        mock->delete_calls++;
        if (mock->delete_error)
                return mock->delete_error;
        /* Consume the pending candidate. */
        memmove (&mock->handles[0], &mock->handles[1],
                (size_t)(--mock->handle_count) * sizeof (uint32_t));
        memmove (&mock->names[0], &mock->names[1],
                (size_t)mock->handle_count * sizeof (const char *));
        return GP_OK;
}

static int
mock_cancelled (void *user_data)
{
        MockReconcile *mock = user_data;

        return mock->cancel_after >= 0 && mock->delete_calls > mock->cancel_after;
}

static void
mock_reset (MockReconcile *mock)
{
        memset (mock, 0, sizeof (*mock));
        mock->cancel_after = -1;
}

int
main (void)
{
        MockReconcile mock;
        PentaxReconcileOps ops;
        char names[4][128];
        int count = -1, ret;

        memset (names, 0, sizeof (names));
        ops.user_data = &mock;
        ops.get_conditions = mock_get_conditions;
        ops.get_candidate_info = mock_get_candidate_info;
        ops.transfer_candidate = mock_transfer_candidate;
        ops.delete_candidate = mock_delete_candidate;
        ops.is_cancelled = mock_cancelled;

        /* 1. No pending candidate: immediate success, zero work. */
        mock_reset (&mock);
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 0);
        CHECK (mock.transfer_calls == 0);
        CHECK (mock.delete_calls == 0);

        /* 2. One pending candidate: consumed and finalized. */
        mock_reset (&mock);
        mock.handles[0] = 100;
        mock.handle_count = 1;
        mock.names[0] = "IMG_9999.ARW";
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (mock.transfer_calls == 1);
        CHECK (mock.delete_calls == 1);
        CHECK (mock.handle_count == 0);
        CHECK (!strcmp (names[0], "IMG_9999.ARW"));

        /* 3. Two pending candidates: both consumed in order. */
        mock_reset (&mock);
        mock.handles[0] = 200;
        mock.handles[1] = 201;
        mock.handle_count = 2;
        mock.names[0] = "IMG_0001.ARW";
        mock.names[1] = "IMG_0001.JPG";
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 2);
        CHECK (mock.transfer_calls == 2);
        CHECK (mock.delete_calls == 2);
        CHECK (!strcmp (names[0], "IMG_0001.ARW"));
        CHECK (!strcmp (names[1], "IMG_0001.JPG"));

        /* RAW+JPEG: an initially empty post-primary sample is not completion.
         * The camera's +524 contract requires the delayed companion. */
        mock_reset (&mock);
        mock.writing_format = 2;
        mock.conditions_empty_first = 1;
        mock.handles[0] = 202;
        mock.handle_count = 1;
        mock.names[0] = "IMG_0002.DNG";
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (mock.conditions_calls >= 2);
        CHECK (mock.transfer_calls == 1);
        CHECK (mock.delete_calls == 1);

        /* 4. Transfer failure: error propagates, count reflects completed. */
        mock_reset (&mock);
        mock.handles[0] = 300;
        mock.handle_count = 1;
        mock.transfer_error = GP_ERROR_IO;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_ERROR_IO);
        CHECK (count == 0);
        CHECK (mock.delete_calls == 0);

        /* 5. Delete failure: error propagates after the transfer. */
        mock_reset (&mock);
        mock.handles[0] = 400;
        mock.handle_count = 1;
        mock.delete_error = GP_ERROR_IO;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_ERROR_IO);
        CHECK (count == 0);
        CHECK (mock.transfer_calls == 1);

        /* 6. Transient conditions failure: retried within the budget. */
        mock_reset (&mock);
        mock.handles[0] = 500;
        mock.handle_count = 1;
        mock.conditions_fail_first = 2;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (mock.conditions_calls >= 3);

        /* 7. Bound exhaustion: more candidates than max_count; the loop
         * stops at the bound and reports success with the remainder left
         * for the next-capture stale-candidate barrier. */
        mock_reset (&mock);
        mock.handles[0] = 600;
        mock.handles[1] = 601;
        mock.handles[2] = 602;
        mock.handle_count = 3;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 2, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 2);
        CHECK (mock.handle_count == 1);
        CHECK (mock.handles[0] == 602);

        /* 8. Cancellation: stops after the requested number of deletes. */
        mock_reset (&mock);
        mock.handles[0] = 700;
        mock.handles[1] = 701;
        mock.handle_count = 2;
        /* Cancel once one candidate has been finalized: the next loop
         * iteration's cancellation check stops the loop. */
        mock.cancel_after = 0;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_ERROR_CANCEL);
        CHECK (count == 1);

        /* 9. Bad parameters: NULL ops or count. */
        count = -1;
        CHECK (pentax_reconcile_extra_candidates (NULL, 4, 60000, 0, names, &count)
                == GP_ERROR_BAD_PARAMETERS);
        CHECK (count == 0);
        CHECK (pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, NULL)
                == GP_ERROR_BAD_PARAMETERS);

        /* 10. Unreadable/short conditions: get_conditions reports a short
         * blob via the GP_OK path is not possible in this mock; instead
         * verify that a failed info read never blocks reconciliation. */
        mock_reset (&mock);
        mock.handles[0] = 800;
        mock.handle_count = 1;
        mock.info_fail = 1;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (names[0][0] == '\0');

        /* 11. RAW+JPEG: an empty gap after primary finalization is not
         * completion. Discover and finalize the delayed companion. */
        mock_reset (&mock);
        mock.handles[0] = 900;
        mock.handle_count = 1;
        mock.names[0] = "IMG_0002.DNG";
        mock.conditions_empty_first = 3;
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 1, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (mock.conditions_calls >= 4);
        CHECK (mock.transfer_calls == 1);
        CHECK (mock.delete_calls == 1);
        CHECK (!strcmp (names[0], "IMG_0002.DNG"));

        /* 12. The camera-reported writing format supplies the minimum output
         * obligation; Pixel Shift exposure count is deliberately irrelevant. */
        {
                unsigned char conditions[528] = {0};
                CHECK (pentax_expected_extra_candidates (conditions, 527) == 0);
                put_u32le (conditions, 524, 0);
                CHECK (pentax_expected_extra_candidates (conditions, sizeof (conditions)) == 0);
                put_u32le (conditions, 524, 1);
                CHECK (pentax_expected_extra_candidates (conditions, sizeof (conditions)) == 0);
                put_u32le (conditions, 524, 2);
                CHECK (pentax_expected_extra_candidates (conditions, sizeof (conditions)) == 1);
                put_u32le (conditions, 524, 3);
                CHECK (pentax_expected_extra_candidates (conditions, sizeof (conditions)) == 0);
        }

        /* 13. o-v12m companion UAF regression: a successfully published
         * companion transfers buffer ownership to the CameraFile (the
         * transfer callback clears the buffer, simulating
         * gp_file_set_data_and_size()).  The reconcile loop must complete
         * without freeing the published bytes again; the published payload
         * must survive the whole loop. */
        mock_reset (&mock);
        mock.publish = 1;
        mock.handles[0] = 1000;
        mock.handle_count = 1;
        mock.names[0] = "IMG_0003.DNG";
        count = -1;
        ret = pentax_reconcile_extra_candidates (&ops, 4, 60000, 0, names, &count);
        CHECK (ret == GP_OK);
        CHECK (count == 1);
        CHECK (mock.published_count == 1);
        /* The published payload must be intact after the loop: a
         * double-free or stale write would corrupt these bytes. */
        CHECK (mock.published[0] != NULL);
        CHECK (!memcmp (mock.published[0], "EXTRA", 5));
        CHECK (!strcmp (names[0], "IMG_0003.DNG"));

        printf ("test-pentax-reconcile: all checks passed\n");
        return 0;
}
