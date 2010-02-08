/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007-2010 Marc Noirot <marc.noirot AT gmail.com>

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

/* JSON FLV file full dump callbacks */
extern int json_on_header(flv_header * header, flv_parser * parser);
extern int json_on_tag(flv_tag * tag, flv_parser * parser);
extern int json_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser);
extern int json_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser);
extern int json_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser);
extern int json_on_prev_tag_size(uint32 size, flv_parser * parser);
extern int json_on_stream_end(flv_parser * parser);

/* JSON FLV file metadata dump callback */
extern int json_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser);
