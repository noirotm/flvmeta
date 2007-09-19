/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007 Marc Noirot <marc.noirot AT gmail.com>

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
#ifndef __FLVMETA_H__
#define __FLVMETA_H__

/* Configuration of the sources */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* chaine de copyright */
#define COPYRIGHT_STR "Copyright 2007 Marc Noirot"

#define LONG_COPYRIGHT_STR PACKAGE_STRING " - " COPYRIGHT_STR

/* CPU configuration*/
#if defined(_M_IX86) || defined(__i386__)
# define ARCH_X86
#endif

#if defined(ARCH_X86) /* more to come... */
# define LITTLE_ENDIAN_ARCH
#else
# define BIG_ENDIAN_ARCH
#endif

#endif /* __FLVMETA_H__ */
