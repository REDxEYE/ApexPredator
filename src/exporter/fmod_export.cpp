// Created by RED on 09.02.2026.

#include "exporter/fmod_export.h"
#include "apex/hashes.h"

#include "platform/logger.h"
#include "utils/path.h"
#include "vorbis_headers.h"

#include "ogg/ogg.h"
#include "utils/simple_fileio.h"
#include "vorbis/codec.h"


typedef struct RIFFHeader {
    char chunk_id[4]; // "RIFF"
    uint32 chunk_size;
    char format[4]; // "WAVE" for WAV files, but FMOD banks might have a different format
} RIFFHeader;

typedef struct RIFFChunk {
    char id[4];
    uint32 size;
} RIFFChunk;

#define R_CHECK(call, message) do { \
    if ((call)!=BUFFER_SUCCESS) { \
        GLog_Error("Error: %s", message); \
        abort(); \
        return; \
    } \
} while (0)

typedef enum FmodAudioType {
    NONE = 0,
    PCM8 = 1,
    PCM16 = 2,
    PCM24 = 3,
    PCM32 = 4,
    PCMFLOAT = 5,
    GCADPCM = 6,
    IMAADPCM = 7,
    VAG = 8,
    HEVAG = 9,
    XMA = 10,
    MPEG = 11,
    CELT = 12,
    AT9 = 13,
    XWMA = 14,
    VORBIS = 15,
    FmodAudioType_FORCE32 = 0x7FFFFFFF,
} FmodAudioType;

static_assert(sizeof(FmodAudioType) == 4, "FmodAudioType should be 4 bytes");

typedef enum FmodSampleChunkType {
    CHANNELS = 1,
    FREQUENCY = 2,
    LOOP = 3,
    COMMENT = 4,
    XMASEEK = 6,
    DSPCOEFF = 7,
    ATRAC9CFG = 9,
    XWMADATA = 10,
    VORBISDATA = 11,
    PEAKVOLUME = 13,
    VORBISINTRALAYERS = 14,
    OPUSDATALEN = 15,
    FmodSampleChunkType_FORCE32 = 0x7FFFFFFF,
} FmodSampleChunkType;

static_assert(sizeof(FmodSampleChunkType) == 4, "FmodSampleChunkType should be 4 bytes");

#pragma pack(push, 1)
typedef struct FSBHeader {
    char id[4];
    uint32 version;
    uint32 sample_count;
    uint32 dir_size;
    uint32 name_size;
    uint32 data_size;
    FmodAudioType audio_type;
    uint32 flags;
    uint32 _null;
    uint8 hash[24];
} FSBHeader;

typedef struct SampleMetadataHeader {
    uint64 has_data: 1;
    uint64 frequency: 4;
    uint64 channel_count: 1; // +1
    uint64 data_offset: 28;
    uint64 samples: 30;
} SampleMetadataHeader;

typedef struct SampleMetadataChunkHeader {
    uint32 has_next_chunk: 1;
    uint32 chunk_size: 24;
    FmodSampleChunkType chunk_type: 7;
} SampleMetadataChunkHeader;

#pragma pack(pop)

typedef struct SampleMetadataChunk {
    SampleMetadataChunkHeader header;
    uint8 *data;
} SampleMetadataChunk;

DYNAMIC_ARRAY_STRUCT(SampleMetadataChunk, SampleMetadataChunk);

typedef struct SampleMetadata {
    uint32 frequency;
    uint32 channel_count;
    uint32 data_offset;
    uint32 samples;
    DynamicArray_SampleMetadataChunk chunks;
} SampleMetadata;

typedef struct Sample {
    SampleMetadata metadata;
    String name;
    uint8 *data;
    uint32 data_size;
} Sample;

uint32 frequencies_remap[] = {0, 8000, 11000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 96000};

DYNAMIC_ARRAY_STRUCT(Sample, Sample);

typedef struct RIFFContext {
    DynamicArray_Sample samples;
} RIFFContext;

void RIFFContext_free(const RIFFContext *self) {
    for (int i = 0; i < DA_size(&self->samples); ++i) {
        Sample *sample = &self->samples.items[i];
        String_free(&sample->name);
        for (int j = 0; j < DA_size(&sample->metadata.chunks); ++j) {
            const SampleMetadataChunk *chunk = &sample->metadata.chunks.items[j];
            mp_free(chunk->data);
        }
        DA_free(&sample->metadata.chunks);
        mp_free(sample->data);
    }
}

static const VorbHeader *vorb_header_find(const uint32_t crc) {
    size_t lo = 0, hi = vorb_headers_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t v = vorb_headers[mid].crc;
        if (v == crc) return &vorb_headers[mid];
        if (v < crc) lo = mid + 1;
        else hi = mid;
    }
    return NULL;
}

static void write_pages(ogg_stream_state *os, MemoryBuffer *out, int (*func)(ogg_stream_state *, ogg_page *)) {
    ogg_page og;
    while (func(os, &og)) {
        R_CHECK(out->write(out, og.header, (size_t)og.header_len, NULL), "Failed to write Ogg page header");
        R_CHECK(out->write(out, og.body, (size_t)og.body_len, NULL), "Failed to write Ogg page body");
    }
}

static int ilog2_u32(uint32_t x) {
    // x is power-of-two here (256, 2048). Return log2(x).
    int r = 0;
    while (x > 1) {
        x >>= 1;
        r++;
    }
    return r;
}

static int build_id_header(ogg_packet *op, const uint8_t channels, const uint32_t rate,
                           const uint32_t block_short, const uint32_t block_long,
                           const uint8_t serialno) {
    oggpack_buffer bp;
    oggpack_writeinit(&bp);

    oggpack_write(&bp, 0x01, 8);
    oggpack_write(&bp, 'v', 8);
    oggpack_write(&bp, 'o', 8);
    oggpack_write(&bp, 'r', 8);
    oggpack_write(&bp, 'b', 8);
    oggpack_write(&bp, 'i', 8);
    oggpack_write(&bp, 's', 8);

    oggpack_write(&bp, 0, 32); // version
    oggpack_write(&bp, channels, 8);
    oggpack_write(&bp, (int) rate, 32);
    oggpack_write(&bp, 0, 32); // bitrate max
    oggpack_write(&bp, 0, 32); // bitrate nominal
    oggpack_write(&bp, 0, 32); // bitrate min

    oggpack_write(&bp, ilog2_u32(block_short), 4);
    oggpack_write(&bp, ilog2_u32(block_long), 4);
    oggpack_write(&bp, 1, 1); // framing flag

    long bytes = oggpack_bytes(&bp);
    uint8_t *pkt = (uint8_t *) malloc((size_t) bytes);
    if (!pkt) {
        oggpack_writeclear(&bp);
        return 0;
    }
    memcpy(pkt, bp.buffer, (size_t) bytes);
    oggpack_writeclear(&bp);

    memset(op, 0, sizeof(*op));
    op->packet = pkt;
    op->bytes = bytes;
    op->b_o_s = 1;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 0;
    (void) serialno;
    return 1;
}

static int build_comment_header(ogg_packet *op) {
    vorbis_comment vc;
    vorbis_comment_init(&vc);

    memset(op, 0, sizeof(*op));
    if (vorbis_commentheader_out(&vc, op) != 0) {
        vorbis_comment_clear(&vc);
        return 0;
    }

    // vorbis_commentheader_out allocates op->packet internally; libvorbis manages it.
    // We'll treat it as owned by libvorbis; to free, use ogg_packet_clear(op) later.
    vorbis_comment_clear(&vc);
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 1;
    return 1;
}

static int build_setup_header(ogg_packet *op, const uint8_t *setup, const uint32_t setup_size) {
    uint8_t *pkt = (uint8_t *) malloc(setup_size);
    if (!pkt) return 0;
    memcpy(pkt, setup, setup_size);

    memset(op, 0, sizeof(*op));
    op->packet = pkt;
    op->bytes = (long) setup_size;
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 2;
    return 1;
}

static uint16_t rd_u16le(const uint8_t *p) {
    return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}

int rebuild_fsb_vorbis_to_ogg(const Sample *s, MemoryBuffer *out) {
    if (!s || !out)
        return 0;

    const SampleMetadata *metadata = &s->metadata;

    uint32 vorbis_crc = 0;
    DA_FORI(metadata->chunks, i) {
        const SampleMetadataChunk *chunk = &metadata->chunks.items[i];
        if (chunk->header.chunk_type == VORBISDATA) {
            vorbis_crc = *((uint32 *) chunk->data);
            break;
        }
    }
    if (vorbis_crc == 0) {
        GLog_Error("No VORBISDATA chunk found in sample metadata");
        return 0;
    }

    const VorbHeader *vh = vorb_header_find(vorbis_crc);
    if (!vh)
        return 0;

    vorbis_info vi;
    vorbis_comment vc;
    vorbis_info_init(&vi);
    vorbis_comment_init(&vc);

    ogg_stream_state os;
    if (ogg_stream_init(&os, 1) != 0) {
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        return 0;
    }

    ogg_packet id = {0}, comment = {0}, setup = {};
    if (!build_id_header(&id, metadata->channel_count, metadata->frequency, 0x100, 0x800, 1) ||
        !build_comment_header(&comment) ||
        !build_setup_header(&setup, vh->data, vh->data_size)) {
        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        free(id.packet);
        ogg_packet_clear(&comment);
        free(setup.packet);
        return 0;
    }

    const int id_res = vorbis_synthesis_headerin(&vi, &vc, &id);
    const int comment_res = vorbis_synthesis_headerin(&vi, &vc, &comment);
    const int setup_res = vorbis_synthesis_headerin(&vi, &vc, &setup);
    if (id_res != 0 || comment_res != 0 || setup_res != 0) {
        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        free(id.packet);
        ogg_packet_clear(&comment);
        free(setup.packet);
        return 0;
    }

    ogg_stream_packetin(&os, &id);
    write_pages(&os, out, ogg_stream_pageout);

    ogg_stream_packetin(&os, &comment);
    write_pages(&os, out, ogg_stream_pageout);

    ogg_stream_packetin(&os, &setup);
    write_pages(&os, out, ogg_stream_pageout);
    write_pages(&os, out, ogg_stream_flush);

    int64_t packetno = setup.packetno;
    int64_t granulepos = 0;
    long prev_blocksize = 0;

    size_t off = 0;
    uint16_t pkt_size = 0;
    if (s->data_size < 2) goto done_ok;

    pkt_size = rd_u16le(s->data + off);
    off += 2;

    while (pkt_size) {
        if (off + pkt_size > s->data_size) {
            // malformed
            goto done_fail;
        }

        packetno++;

        ogg_packet op;
        memset(&op, 0, sizeof(op));

        uint8_t *pkt = (uint8_t *) malloc(pkt_size);
        if (!pkt)
            goto done_fail;
        memcpy(pkt, s->data + off, pkt_size);
        off += pkt_size;

        op.packet = pkt;
        op.bytes = (long) pkt_size;
        op.packetno = packetno;

        // next size (0 means eos). If EOF, treat as eos.
        uint16_t next_size = 0;
        if (off + 2 <= s->data_size) {
            next_size = rd_u16le(s->data + off);
            off += 2;
        }
        else {
            next_size = 0;
        }
        op.e_o_s = next_size ? 0 : 1;

        long bs = vorbis_packet_blocksize(&vi, &op);
        if (bs <= 0) {
            free(pkt);
            goto done_fail;
        }

        if (prev_blocksize) {
            granulepos += (bs + prev_blocksize) / 4;
        }
        else {
            granulepos = 0;
        }
        op.granulepos = granulepos;
        prev_blocksize = bs;

        ogg_stream_packetin(&os, &op);
        write_pages(&os, out, ogg_stream_pageout);

        free(pkt);
        pkt_size = next_size;
    }

done_ok:
    // cleanup
    free(id.packet);
    ogg_packet_clear(&comment);
    free(setup.packet);

    ogg_stream_clear(&os);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    return 1;

done_fail:
    free(id.packet);
    ogg_packet_clear(&comment);
    free(setup.packet);

    ogg_stream_clear(&os);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    return 0;
}

void process_fsb(RIFFContext *ctx, MemoryBuffer *mb) {
    FSBHeader header;
    R_CHECK(mb->read(mb, &header, sizeof(FSBHeader), NULL), "Failed to read FSB header from FMOD bank");

    const uint32 header_size = header.version == 0 ? 0x40 : 0x3C;

    MemoryBuffer *dir_buffer = MemoryBuffer_new();
    R_CHECK(MemoryBuffer_make_sub_buffer(dir_buffer, mb, header_size, header.dir_size),
            "Failed to create sub-buffer for FSB directory in FMOD bank");
    mb->skip(mb, header.dir_size);
    DA_init(&ctx->samples, Sample, header.sample_count);

    for (int i = 0; i < header.sample_count; ++i) {
        SampleMetadataHeader metadata_header;
        R_CHECK(dir_buffer->read(dir_buffer, &metadata_header, sizeof(SampleMetadataHeader), NULL),
                "Failed to read sample metadata header from FSB directory in FMOD bank");
        Sample *sample = (Sample *)DA_append_get(&ctx->samples);
        SampleMetadata *metadata = &sample->metadata;

        metadata->frequency = frequencies_remap[metadata_header.frequency];
        metadata->channel_count = metadata_header.channel_count + 1;
        metadata->data_offset = metadata_header.data_offset * 16;
        metadata->samples = metadata_header.samples;
        DA_init(&metadata->chunks, SampleMetadataChunk, 4);
        while (metadata_header.has_data) {
            SampleMetadataChunk *chunk = (SampleMetadataChunk *)DA_append_get(&metadata->chunks);
            R_CHECK(dir_buffer->read(dir_buffer, &chunk->header, sizeof(SampleMetadataChunkHeader), NULL),
                    "Failed to read sample metadata chunk header from FSB directory in FMOD bank");
            chunk->data = (uint8*)mp_malloc(chunk->header.chunk_size);
            R_CHECK(dir_buffer->read(dir_buffer, chunk->data, chunk->header.chunk_size, NULL),
                    "Failed to read sample metadata chunk data from FSB directory in FMOD bank");

            if (chunk->header.chunk_type == FREQUENCY) {
                metadata_header.frequency = *(uint32 *) chunk->data;
            }

            if (!chunk->header.has_next_chunk) break;
        }
    }
    uint32 *name_offsets = (uint32 *)mp_malloc(header.sample_count * sizeof(uint32));
    R_CHECK(mb->read(mb, name_offsets, header.sample_count * sizeof(uint32), NULL),
            "Failed to read sample name offsets from FSB in FMOD bank");
    for (int i = 0; i < header.sample_count; ++i) {
        name_offsets[i] -= header.sample_count * sizeof(uint32);
    }

    char *names = (char *)mp_malloc(header.name_size - header.sample_count * sizeof(uint32));
    R_CHECK(mb->read(mb, names, header.name_size - header.sample_count * sizeof(uint32), NULL),
            "Failed to read sample names from FSB in FMOD bank");

    DA_FORI(ctx->samples, i) {
        const Sample *sample = &ctx->samples.items[i];
        GLog_Info("Sample %d: freq=%u, channels=%u, data_offset=%u, samples=%u", i, sample->metadata.frequency,
                   sample->metadata.channel_count, sample->metadata.data_offset, sample->metadata.samples);
        if (sample->metadata.data_offset>header.data_size) {
            GLog_Error("Sample %d has invalid data offset %u (data size is %u)", i, sample->metadata.data_offset,
                       header.data_size);
        }
    }

    MemoryBuffer samples_data = {};
    MemoryBuffer_make_sub_buffer(&samples_data, mb, header_size + header.dir_size + header.name_size, header.data_size);
    R_CHECK(mb->skip(mb, header.data_size), "Failed to skip sample data in FSB in FMOD bank");
    for (int i = 0; i < header.sample_count; ++i) {
        Sample *sample = &ctx->samples.items[i];
        String_from_cstr(&sample->name, names + name_offsets[i]);

        const uint32 sample_start = sample->metadata.data_offset;
        uint32 sample_end;
        if (i < header.sample_count - 1) {
            sample_end = ctx->samples.items[i + 1].metadata.data_offset;
        }else {
            sample_end = header.data_size;
        }
        const uint32 sample_size = sample_end - sample_start;
        sample->data = (uint8*)mp_malloc(sample_size);
        memcpy(sample->data, samples_data.data + sample_start, sample_size);
        sample->data_size = sample_size;
    }
}

void export_fmod_bank(const AppState *app_state, const uint32 path_hash, MemoryBuffer *buffer) {
    CHECK_APP_STATE(app_state);

    String bank_output_path = {};
    String_copy_from(&bank_output_path, &app_state->export_path);

    const StringView bank_name = find_name32_sv(path_hash);
    if (sv_is_not_null(bank_name)) {
        Path_join_sv(&bank_output_path, bank_name);
        Path_replace_extension_inplace(&bank_output_path, "");
    }
    else {
        Path_join_cstr(&bank_output_path, "bank_");
        String_append_format(&bank_output_path, "%08X", path_hash);
    }
    Path_ensure_dirs(&bank_output_path);

    RIFFHeader riff_header;
    R_CHECK(buffer->read(buffer, &riff_header, sizeof(RIFFHeader), NULL), "Failed to read RIFF header from FMOD bank");

    if (memcmp(riff_header.chunk_id, "RIFF", 4) != 0) {
        GLog_Error("Invalid RIFF header in FMOD bank with hash %u", path_hash);
        return;
    }
    if (memcmp(riff_header.format, "FEV ", 4) != 0) {
        GLog_Error("Unexpected RIFF format in FMOD bank with hash %u: expected 'FEV ', got '%.4s'", path_hash,
                   riff_header.format);
        return;
    }
    RIFFContext ctx = {};

    while (buffer->position < buffer->size) {
        // process_chunk(&ctx, (Buffer*)buffer);
        RIFFChunk chunk;
        R_CHECK(buffer->read(buffer, &chunk,sizeof(RIFFChunk), NULL), "Failed to read RIFF chunk header from FMOD bank");
        if (memcmp(chunk.id, "SND ", 4) == 0) {
            int64 data_offset;
            R_CHECK(buffer->get_position(buffer, &data_offset), "Failed to get current position in FMOD bank for SND chunk");
            // Align offset to 16
            const int64 aligned_offset = (data_offset + 31) & ~31;
            const int64 fsb_size = chunk.size - (aligned_offset - data_offset);
            MemoryBuffer *fsb_subbuffer = MemoryBuffer_new();
            R_CHECK(MemoryBuffer_make_sub_buffer(fsb_subbuffer, buffer, aligned_offset, fsb_size),
                    "Failed to create sub-buffer for SND chunk data in FMOD bank");
            R_CHECK(buffer->skip(buffer, chunk.size), "Failed to skip SND chunk data in FMOD bank");
            process_fsb(&ctx, fsb_subbuffer);
        }
        else {
            R_CHECK(buffer->skip(buffer, chunk.size), "Failed to skip RIFF chunk data in FMOD bank");
        }
    }
    Path_ensure_dirs(&bank_output_path);
    DA_FORI(ctx.samples, i) {
        const Sample *sample = &ctx.samples.items[i];
        String sample_output_path = {};
        String_copy_from(&sample_output_path, &bank_output_path);
        Path_join(&sample_output_path, &sample->name);
        String_append_cstr(&sample_output_path, ".ogg");

        Path_ensure_parent_dirs(&sample_output_path);

        MemoryBuffer *ogg_buffer = MemoryBuffer_new();
        MemoryBuffer_allocate(ogg_buffer, 1024 * 1024); // 1 MB initial size, will grow if needed
        if (rebuild_fsb_vorbis_to_ogg(sample, ogg_buffer)) {

            if (!write_file(String_cstr(&sample_output_path), ogg_buffer->data, ogg_buffer->position)) {
                GLog_Error("Failed to write Ogg file for sample in FMOD bank: %s", String_cstr(&sample_output_path));
            }
            ogg_buffer->close(ogg_buffer);
        }
        else {
            GLog_Error("Failed to rebuild Ogg data for sample in FMOD bank: %s", String_cstr(&sample->name));
            ogg_buffer->close(ogg_buffer);
        }
        String_free(&sample_output_path);
    }

    RIFFContext_free(&ctx);

    buffer->close(buffer);
}
