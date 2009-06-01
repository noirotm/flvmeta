/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008, 2009 Marc Noirot <marc.noirot AT gmail.com>

    This file is part of FLVMeta.

    FLVMeta is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLVMeta is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLVMeta; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "flvmeta.h"
#include "flv.h"

#include <string.h>

void flv_tag_set_timestamp(flv_tag * tag, uint32 timestamp) {
    tag->timestamp = uint32_to_uint24_be(timestamp);
    tag->timestamp_extended = (uint8)((timestamp & 0xFF000000) >> 24);
}

/* FLV stream functions */
flv_stream * flv_open(const char * file) {
    flv_stream * stream = malloc(sizeof(flv_stream));
    if (stream == NULL) {
        return NULL;
    }
    stream->flvin = fopen(file, "rb");
    if (stream->flvin == NULL) {
        free(stream);
        return NULL;
    }
    stream->current_tag_body_length = 0;
    stream->current_tag_offset = 0;
    stream->state = FLV_STREAM_STATE_START;
    return stream;
}

int flv_read_header(flv_stream * stream, flv_header * header) {
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)
    || stream->state != FLV_STREAM_STATE_START
    || fread(header, sizeof(flv_header), 1, stream->flvin) == 0) {
        return ERROR_EOF;
    }
    
    stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    return OK;
}

int flv_read_prev_tag_size(flv_stream * stream, uint32 * prev_tag_size) {
    uint32_be val;
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)) {
        return ERROR_EOF;
    }

    /* skip remaining tag body bytes */
    if (stream->state == FLV_STREAM_STATE_TAG_BODY) {
        flvmeta_fseek(stream->flvin, stream->current_tag_offset + sizeof(flv_tag) + uint24_be_to_uint32(stream->current_tag.body_length), SEEK_SET);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    if (stream->state == FLV_STREAM_STATE_PREV_TAG_SIZE) {
        if (fread(&val, sizeof(uint32_be), 1, stream->flvin) == 0) {
            return ERROR_EOF;
        }
        else {
            stream->state = FLV_STREAM_STATE_TAG;
            *prev_tag_size = swap_uint32(val);
            return OK;
        }
    }
    else {
        return ERROR_EOF;
    }
}

int flv_read_tag(flv_stream * stream, flv_tag * tag) {
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)) {
        return ERROR_EOF;
    }

    /* skip header */
    if (stream->state == FLV_STREAM_STATE_START) {
        flvmeta_fseek(stream->flvin, sizeof(flv_header), SEEK_CUR);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    /* skip current tag body */
    if (stream->state == FLV_STREAM_STATE_TAG_BODY) {
        flvmeta_fseek(stream->flvin, stream->current_tag_offset + sizeof(flv_tag) + uint24_be_to_uint32(stream->current_tag.body_length), SEEK_SET);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    /* skip previous tag size */
    if (stream->state == FLV_STREAM_STATE_PREV_TAG_SIZE) {
        flvmeta_fseek(stream->flvin, sizeof(uint32_be), SEEK_CUR);
        stream->state = FLV_STREAM_STATE_TAG;
    }
    
    if (stream->state == FLV_STREAM_STATE_TAG) {
        stream->current_tag_offset = flvmeta_ftell(stream->flvin);
        if (fread(tag, sizeof(flv_tag), 1, stream->flvin) == 0) {
            return ERROR_EOF;
        }
        else {
            memcpy(&stream->current_tag, tag, sizeof(flv_tag));
            stream->current_tag_body_length = uint24_be_to_uint32(tag->body_length);
            stream->state = FLV_STREAM_STATE_TAG_BODY;
            return OK;
        }
    }
    else {
        return ERROR_EOF;
    }
}

int flv_read_audio_tag(flv_stream * stream, flv_audio_tag * tag) {
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)
    || stream->state != FLV_STREAM_STATE_TAG_BODY
    || fread(tag, sizeof(flv_audio_tag), 1, stream->flvin) == 0) {
        return ERROR_EOF;
    }
    
    stream->current_tag_body_length -= sizeof(flv_audio_tag);

    if (stream->current_tag_body_length <= 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    return OK;
}

int flv_read_video_tag(flv_stream * stream, flv_video_tag * tag) {
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)
    || stream->state != FLV_STREAM_STATE_TAG_BODY
    || fread(tag, sizeof(flv_video_tag), 1, stream->flvin) == 0) {
        return ERROR_EOF;
    }

    stream->current_tag_body_length -= sizeof(flv_video_tag);

    if (stream->current_tag_body_length <= 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    return OK;
}

int flv_read_metadata(flv_stream * stream, amf_data ** name, amf_data ** data) {
    amf_data * d;
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)
    || stream->state != FLV_STREAM_STATE_TAG_BODY) {
        return ERROR_EOF;
    }
    
    /* read metadata name */
    d = amf_data_file_read(stream->flvin);
    if (d == NULL) {
        return ERROR_EOF;
    }
    *name = d;
    stream->current_tag_body_length -= amf_data_size(d);
    
    /* read metadata contents */
    d = amf_data_file_read(stream->flvin);
    if (d == NULL) {
        amf_data_free(*name);
        return ERROR_EOF;
    }
    *data = d;
    stream->current_tag_body_length -= amf_data_size(d);

    if (stream->current_tag_body_length <= 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    return OK;
}

size_t flv_read_tag_body(flv_stream * stream, byte * buffer, size_t buffer_size) {
    size_t bytes_number;
    if (stream == NULL
    || stream->flvin == NULL
    || feof(stream->flvin)
    || stream->state != FLV_STREAM_STATE_TAG_BODY) {
        return 0;
    }

    bytes_number = (buffer_size > stream->current_tag_body_length) ? stream->current_tag_body_length : buffer_size;
    bytes_number = fread(buffer, sizeof(byte), bytes_number, stream->flvin);
    
    stream->current_tag_body_length -= bytes_number;

    if (stream->current_tag_body_length == 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    return bytes_number;
}

void flv_close(flv_stream * stream) {
    if (stream != NULL) {
        if (stream->flvin != NULL) {
            fclose(stream->flvin);
        }
        free(stream);
    }
}

/* FLV event based parser */
int flv_parse(const char * file, flv_parser * parser) {
    flv_header header;
    flv_tag tag;
    flv_audio_tag at;
    flv_video_tag vt;
    amf_data * name, * data;
    uint32 prev_tag_size;
    int retval;

    if (parser == NULL) {
        return ERROR_EOF;
    }

    parser->stream = flv_open(file);
    if (parser->stream == NULL) {
        return ERROR_OPEN_READ;
    }

    retval = flv_read_header(parser->stream, &header);
    if (retval != OK) {
        flv_close(parser->stream);
        return retval;
    }

    if (parser->on_header != NULL) {
        retval = parser->on_header(&header, parser);
        if (retval != OK) {
            flv_close(parser->stream);
            return retval;
        }
    }

    while (flv_read_tag(parser->stream, &tag) == OK) {
        if (parser->on_tag != NULL) {
            retval = parser->on_tag(&tag, parser);
            if (retval != OK) {
                flv_close(parser->stream);
                return retval;
            }
        }

        if (tag.type == FLV_TAG_TYPE_AUDIO) {
            retval = flv_read_audio_tag(parser->stream, &at);
            if (retval != OK) {
                flv_close(parser->stream);
                return retval;
            }
            if (parser->on_audio_tag != NULL) {
                retval = parser->on_audio_tag(&tag, at, parser);
                if (retval != OK) {
                    flv_close(parser->stream);
                    return retval;
                }
            }
        }
        else if (tag.type == FLV_TAG_TYPE_VIDEO) {
            retval = flv_read_video_tag(parser->stream, &vt);
            if (retval != OK) {
                flv_close(parser->stream);
                return retval;
            }
            if (parser->on_video_tag != NULL) {
                retval = parser->on_video_tag(&tag, vt, parser);
                if (retval != OK) {
                    flv_close(parser->stream);
                    return retval;
                }
            }
        }
        else if (tag.type == FLV_TAG_TYPE_META) {
            retval = flv_read_metadata(parser->stream, &name, &data);
            if (retval != OK) {
                flv_close(parser->stream);
                return retval;
            }
            if (parser->on_metadata_tag != NULL) {
                retval = parser->on_metadata_tag(&tag, name, data, parser);
                if (retval != OK) {
                    amf_data_free(name);
                    amf_data_free(data);
                    flv_close(parser->stream);
                    return retval;
                }
            }
            amf_data_free(name);
            amf_data_free(data);
        }
        else {
            if (parser->on_unknown_tag != NULL) {
                retval = parser->on_unknown_tag(&tag, parser);
                if (retval != OK) {
                    flv_close(parser->stream);
                    return retval;
                }
            }
        }
        retval = flv_read_prev_tag_size(parser->stream, &prev_tag_size);
        if (retval != OK) {
            flv_close(parser->stream);
            return retval;
        }
        if (parser->on_prev_tag_size != NULL) {
            retval = parser->on_prev_tag_size(prev_tag_size, parser);
            if (retval != OK) {
                flv_close(parser->stream);
                return retval;
            }
        }
    }
    
    if (parser->on_stream_end != NULL) {
        retval = parser->on_stream_end(parser);
        if (retval != OK) {
            flv_close(parser->stream);
            return retval;
        }
    }

    flv_close(parser->stream);
    return OK;
}
