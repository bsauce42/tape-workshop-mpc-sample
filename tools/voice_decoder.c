// SPDX-License-Identifier: AGPL-3.0-only
#include <pocketsphinx.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

__attribute__((visibility("default")))
int tape_workshop_recognize(const char *folder, const int16 *audio, size_t count,
                            char *result, size_t capacity)
{
    char model[4096], dictionary[4096], words[4096];
    if (snprintf(model, sizeof model, "%s/en-us", folder) >= (int)sizeof model ||
        snprintf(dictionary, sizeof dictionary, "%s/words.dict", folder) >= (int)sizeof dictionary ||
        snprintf(words, sizeof words, "%s/words.kws", folder) >= (int)sizeof words)
        return -1;
    ps_config_t *config = ps_config_init(NULL);
    if (!config)
        return -1;
    ps_config_set_str(config, "hmm", model);
    ps_config_set_str(config, "dict", dictionary);
    ps_config_set_str(config, "kws", words);
    ps_config_set_str(config, "loglevel", "ERROR");
    ps_decoder_t *decoder = ps_init(config);
    ps_config_free(config);
    if (!decoder)
        return -1;
    int status = -1;
    if (ps_start_utt(decoder) == 0 &&
        ps_process_raw(decoder, audio, count, 0, 1) >= 0 &&
        ps_end_utt(decoder) == 0) {
        int32 best = INT_MIN;
        status = 0;
        for (ps_seg_t *seg = ps_seg_iter(decoder); seg; seg = ps_seg_next(seg)) {
            int start, end;
            ps_seg_frames(seg, &start, &end);
            const int32 confidence = ps_seg_prob(seg, NULL, NULL, NULL);
            if (end - start < 10 || confidence <= best)
                continue;
            best = confidence;
            const char *word = ps_seg_word(seg);
            const int length = (int)strcspn(word, " \t");
            status = snprintf(result, capacity, "%.*s", length, word) < (int)capacity ? 1 : -1;
        }
    }
    ps_free(decoder);
    return status;
}
