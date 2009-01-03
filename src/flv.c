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

void flv_tag_set_timestamp(flv_tag * tag, uint32 timestamp) {
    tag->timestamp = uint32_to_uint24_be(timestamp);
    tag->timestamp_extended = (uint8)((timestamp & 0xFF000000) >> 24);
}
