// Created by RED on 09.02.2026.

#include "exporter/fmod_export.h"
#include "apex/hashes.h"

#include "redscore/platform/logger.h"
#include "vorbis_headers.h"

#include "ogg/ogg.h"
#include "redscore/utils/simple_fileio.h"
#include "vorbis/codec.h"

#include <vector>
#include <ranges>


struct RIFFHeader {
    char chunk_id[4]; // "RIFF"
    uint32 chunk_size;
    char format[4]; // "WAVE" for WAV files, but FMOD banks might have a different format
};

struct RIFFChunk {
    char id[4];
    uint32 size;
};

enum class FmodAudioType: uint32 {
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
};

static_assert(sizeof(FmodAudioType) == 4, "FmodAudioType should be 4 bytes");

enum class FmodSampleChunkType: uint32 {
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
};

static_assert(sizeof(FmodSampleChunkType) == 4, "FmodSampleChunkType should be 4 bytes");

#pragma pack(push, 1)
struct FSBHeader {
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
};

struct SampleMetadataHeader {
    uint64 has_data: 1;
    uint64 frequency: 4;
    uint64 channel_count: 1; // +1
    uint64 data_offset: 28;
    uint64 samples: 30;
};

struct SampleMetadataChunkHeader {
    uint32 has_next_chunk: 1;
    uint32 chunk_size: 24;
    FmodSampleChunkType chunk_type: 7;
};

#pragma pack(pop)

struct SampleMetadataChunk {
    SampleMetadataChunkHeader header{};
    IO::Buffer data;
};

struct SampleMetadata {
    uint32 frequency{};
    uint32 channel_count{};
    uint32 data_offset{};
    uint32 samples{};
    std::vector<SampleMetadataChunk> chunks;
};

struct Sample {
    SampleMetadata metadata;
    std::string name;
    std::vector<uint8> data;
};

uint32 frequencies_remap[] = {0, 8000, 11000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 96000};

struct RIFFContext {
    std::vector<Sample> samples;
};

static const VorbHeader *vorb_header_find(const uint32_t crc) {
    size_t lo = 0, hi = vorb_headers_count;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const uint32_t v = vorb_headers[mid].crc;
        if (v == crc) return &vorb_headers[mid];
        if (v < crc) lo = mid + 1;
        else hi = mid;
    }
    return nullptr;
}

// int(*func)(ogg_stream_state *, ogg_page *)
using WriteFunc = int(*)(ogg_stream_state *, ogg_page *);

static void write_pages(ogg_stream_state &os, const std::unique_ptr<IO::MemoryFile> &out, const WriteFunc func) {
    ogg_page og;
    while (func(&os, &og)) {
        out->write(og.header, og.header_len);
        out->write(og.body, og.body_len);
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
    auto *pkt = static_cast<uint8_t *>(malloc(setup_size));
    if (!pkt) return 0;
    memcpy(pkt, setup, setup_size);

    memset(op, 0, sizeof(*op));
    op->packet = pkt;
    op->bytes = static_cast<long>(setup_size);
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 2;
    return 1;
}

static uint16_t rd_u16le(const uint8_t *p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

int rebuild_fsb_vorbis_to_ogg(const Sample &sample, const std::unique_ptr<IO::MemoryFile> &out) {
    const SampleMetadata &metadata = sample.metadata;

    uint32 vorbis_crc = 0;
    for (const auto &chunk: metadata.chunks) {
        if (chunk.header.chunk_type == FmodSampleChunkType::VORBISDATA) {
            const auto view = chunk.data.readonly_view_as<uint32>();
            vorbis_crc = view[0];
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

    ogg_packet id = {nullptr}, comment = {nullptr}, setup = {};
    if (!build_id_header(&id, metadata.channel_count, metadata.frequency, 0x100, 0x800, 1) ||
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
    write_pages(os, out, ogg_stream_pageout);

    ogg_stream_packetin(&os, &comment);
    write_pages(os, out, ogg_stream_pageout);

    ogg_stream_packetin(&os, &setup);
    write_pages(os, out, ogg_stream_pageout);
    write_pages(os, out, ogg_stream_flush);

    int64_t packetno = setup.packetno;
    int64_t granulepos = 0;
    long prev_blocksize = 0;

    size_t off = 0;
    uint16_t pkt_size = 0;
    if (sample.data.size() < 2) goto done_ok;

    pkt_size = rd_u16le(sample.data.data() + off);
    off += 2;

    while (pkt_size) {
        if (off + pkt_size > sample.data.size()) {
            // malformed
            goto done_fail;
        }

        packetno++;

        ogg_packet op;
        memset(&op, 0, sizeof(op));

        auto *pkt = static_cast<uint8_t *>(malloc(pkt_size));
        if (!pkt)
            goto done_fail;
        memcpy(pkt, sample.data.data() + off, pkt_size);
        off += pkt_size;

        op.packet = pkt;
        op.bytes = (long) pkt_size;
        op.packetno = packetno;

        // next size (0 means eos). If EOF, treat as eos.
        uint16_t next_size = 0;
        if (off + 2 <= sample.data.size()) {
            next_size = rd_u16le(sample.data.data() + off);
            off += 2;
        }
        else {
            next_size = 0;
        }
        op.e_o_s = next_size ? 0 : 1;

        const long bs = vorbis_packet_blocksize(&vi, &op);
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
        write_pages(os, out, ogg_stream_pageout);

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

void process_fsb(RIFFContext &ctx, const std::unique_ptr<IO::File> &file) {
    const auto header = file->read_pod<FSBHeader>();

    const uint32 header_size = header.version == 0 ? 0x40 : 0x3C;
    file->set_position(header_size);
    IO::Buffer dir_buffer(header.dir_size);
    file->read_exact(dir_buffer.as_span());
    const auto dir_file = std::make_unique<IO::MemoryViewFile>(dir_buffer);
    ctx.samples.reserve(header.sample_count);

    for (int i = 0; i < header.sample_count; ++i) {
        auto metadata_header = dir_file->read_pod<SampleMetadataHeader>();
        auto &sample = ctx.samples.emplace_back();
        SampleMetadata &metadata = sample.metadata;

        metadata.frequency = frequencies_remap[metadata_header.frequency];
        metadata.channel_count = metadata_header.channel_count + 1;
        metadata.data_offset = metadata_header.data_offset * 16;
        metadata.samples = metadata_header.samples;
        metadata.chunks.reserve(4);

        while (metadata_header.has_data) {
            SampleMetadataChunk &chunk = metadata.chunks.emplace_back();
            chunk.header = dir_file->read_pod<SampleMetadataChunkHeader>();
            chunk.data.resize(chunk.header.chunk_size);
            dir_file->read_exact(chunk.data.as_span());
            if (chunk.header.chunk_type == FmodSampleChunkType::FREQUENCY) {
                metadata_header.frequency = chunk.data.readonly_view_as<uint32>().at(0);
            }

            if (!chunk.header.has_next_chunk) break;
        }
    }
    std::vector<uint32> name_offsets(header.sample_count);
    file->read_exact(name_offsets);
    for (int i = 0; i < header.sample_count; ++i) {
        name_offsets[i] -= header.sample_count * sizeof(uint32);
    }

    std::vector<char> names(header.name_size - header.sample_count * sizeof(uint32));
    file->read_exact(names);

    IO::Buffer samples_data(header.data_size);
    file->read_exact(samples_data.as_span());
    for (int i = 0; i < header.sample_count; ++i) {
        Sample &sample = ctx.samples[i];
        sample.name = names.data() + name_offsets[i];

        const uint32 sample_start = sample.metadata.data_offset;
        uint32 sample_end;
        if (i < header.sample_count - 1) {
            sample_end = ctx.samples[i + 1].metadata.data_offset;
        }
        else {
            sample_end = header.data_size;
        }
        const uint32 sample_size = sample_end - sample_start;
        sample.data.resize(sample_size);
        memcpy(sample.data.data(), samples_data.data() + sample_start, sample_size);
    }

    for (const auto &[i, sample]: ctx.samples | std::views::enumerate) {
        GLog_Info("Sample \"{}\" ({}): freq={}, channels={}, data_offset={}, samples={}", sample.name, i,
                  sample.metadata.frequency,
                  sample.metadata.channel_count, sample.metadata.data_offset, sample.metadata.samples);
        if (sample.metadata.data_offset > header.data_size) {
            GLog_Error("Sample {} has invalid data offset {} (data size is {})", i, sample.metadata.data_offset,
                       header.data_size);
        }
    }
}

void export_fmod_bank(const ApexAppState &app_state, uint32 path_hash, const std::unique_ptr<IO::File> &&buffer) {
    std::filesystem::path bank_path = find_name(path_hash).value_or(std::format("bank_{:08X}", path_hash));
    bank_path.replace_extension("");

    std::filesystem::path bank_output_path = app_state.export_path() / bank_path;

    std::filesystem::create_directories(bank_path.parent_path());

    const auto riff_header = buffer->read_pod<RIFFHeader>();

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
    while (buffer->remaining() > 0) {
        const auto chunk = buffer->read_pod<RIFFChunk>();
        if (memcmp(chunk.id, "SND ", 4) == 0) {
            const auto data_offset = buffer->get_position();
            const auto aligned_offset = (data_offset + 31) & ~31;
            const auto fsb_size = chunk.size - (aligned_offset - data_offset);
            IO::Buffer fsb_subbuffer(fsb_size);
            buffer->set_position(aligned_offset);
            buffer->read_exact(fsb_subbuffer.as_span());
            process_fsb(ctx, std::make_unique<IO::MemoryViewFile>(fsb_subbuffer));
        }
        else {
            buffer->skip(chunk.size);
        }
    }

    for (const auto &sample: ctx.samples) {
        auto sample_output_path = bank_output_path / sample.name;
        sample_output_path.replace_extension("ogg");
        std::filesystem::create_directories(sample_output_path.parent_path());

        auto ogg_buffer = std::make_unique<IO::MemoryFile>(1024 * 1024);
        if (rebuild_fsb_vorbis_to_ogg(sample, ogg_buffer)) {
            ogg_buffer->buffer().resize(ogg_buffer->get_position());
            write_file(sample_output_path, ogg_buffer->cbuffer());
            ogg_buffer->close();
        }
        else {
            GLog_Error("Failed to rebuild Ogg data for sample in FMOD bank: {}", sample.name);
            ogg_buffer->close();
        }
    }

    buffer->close();
}
